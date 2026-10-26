<#
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Stakeforge is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

Stakeforge is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Stakeforge. If not, see <https://www.gnu.org/licenses/>.

As an additional permission under section 7 of GPLv3, the copyright
holders grant the Stakeforge Game Linking Exception, version 1.0,
in GAME-LINKING-EXCEPTION.md.

#>

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SolutionPath
)

$ErrorActionPreference = 'Stop'
$deadline = [DateTime]::UtcNow.AddSeconds(90)

function Invoke-VisualStudio {
    param([scriptblock]$Action)

    while ($true) {
        try {
            return & $Action
        }
        catch {
            $failure = $_.Exception

            while ($null -ne $failure.InnerException) {
                $failure = $failure.InnerException
            }

            if (($failure.HResult -ne -2147418111 -and $failure.HResult -ne -2147417846) -or [DateTime]::UtcNow -ge $deadline) {
                throw
            }

            Start-Sleep -Milliseconds 250
        }
    }
}

Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.ComTypes;

namespace Stakeforge
{
    public static class VisualStudioAutomation
    {
        [DllImport("user32.dll", SetLastError = true)]
        public static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);

        [DllImport("ole32.dll")]
        private static extern int GetRunningObjectTable(int reserved, out IRunningObjectTable table);

        [DllImport("ole32.dll")]
        private static extern int CreateBindCtx(int reserved, out IBindCtx context);

        public static object[] GetInstances(string programId)
        {
            IRunningObjectTable table = null;
            IBindCtx context = null;
            IEnumMoniker enumerator = null;
            var instances = new List<object>();

            try
            {
                Marshal.ThrowExceptionForHR(GetRunningObjectTable(0, out table));
                Marshal.ThrowExceptionForHR(CreateBindCtx(0, out context));
                table.EnumRunning(out enumerator);
                var monikers = new IMoniker[1];

                while (enumerator.Next(1, monikers, IntPtr.Zero) == 0)
                {
                    try
                    {
                        string name = null;
                        monikers[0].GetDisplayName(context, null, out name);

                        if (name.StartsWith("!" + programId + ":", StringComparison.OrdinalIgnoreCase))
                        {
                            object instance = null;
                            table.GetObject(monikers[0], out instance);
                            instances.Add(instance);
                        }
                    }
                    catch (COMException)
                    {
                    }
                    finally
                    {
                        Marshal.ReleaseComObject(monikers[0]);
                    }
                }

                return instances.ToArray();
            }
            finally
            {
                if (enumerator != null) Marshal.ReleaseComObject(enumerator);
                if (context != null) Marshal.ReleaseComObject(context);
                if (table != null) Marshal.ReleaseComObject(table);
            }
        }
    }
}
'@

try {
    $solution = (Resolve-Path -LiteralPath $SolutionPath).Path
    $settingsPath = Join-Path $PSScriptRoot '../../build_settings.json'
    $settings = Get-Content -LiteralPath $settingsPath -Raw | ConvertFrom-Json
    $generator = [regex]::Match($settings.generator, '^Visual Studio (\d+) ')

    if (-not $generator.Success) {
        throw 'The SDK does not specify a Visual Studio generator.'
    }

    $programId = 'VisualStudio.DTE.' + $generator.Groups[1].Value + '.0'
    $dte = $null

    foreach ($candidate in [Stakeforge.VisualStudioAutomation]::GetInstances($programId)) {
        $candidateSolution = Invoke-VisualStudio { $candidate.Solution.FullName }

        if ([string]::Equals($candidateSolution, $solution, [StringComparison]::OrdinalIgnoreCase)) {
            $dte = $candidate
            break
        }
    }

    if ($null -eq $dte) {
        $automationType = [Type]::GetTypeFromProgID($programId, $true)
        $dte = Invoke-VisualStudio { [Activator]::CreateInstance($automationType) }
        Invoke-VisualStudio { $dte.UserControl = $true }
        Invoke-VisualStudio { $dte.MainWindow.Visible = $true }
        Invoke-VisualStudio { $dte.Solution.Open($solution) }
    }

    Invoke-VisualStudio { $dte.MainWindow.Activate() }
    $ideExecutable = Invoke-VisualStudio { $dte.FullName }
    $assemblyPath = Join-Path (Split-Path -Parent $ideExecutable) 'PublicAssemblies/envdte.dll'
    $automationAssembly = [Reflection.Assembly]::LoadFrom($assemblyPath)
    $windowProperty = $automationAssembly.GetType('EnvDTE.Window', $true).GetProperty('HWnd')
    $mainWindow = Invoke-VisualStudio { $dte.MainWindow }
    $window = Invoke-VisualStudio { $windowProperty.GetValue($mainWindow) }
    [uint32]$ideProcessId = 0

    if ([Stakeforge.VisualStudioAutomation]::GetWindowThreadProcessId([IntPtr]$window, [ref]$ideProcessId) -eq 0) {
        throw 'Could not identify the Visual Studio process.'
    }

    $ideProcess = [Diagnostics.Process]::GetProcessById($ideProcessId)
    $creationTime = $ideProcess.StartTime.ToFileTimeUtc()

    if ($ideProcess.HasExited) {
        throw 'Visual Studio exited while opening the solution.'
    }

    @{ process_id = $ideProcessId; creation_time = $creationTime } | ConvertTo-Json -Compress
    exit 0
}
catch {
    [Console]::Error.WriteLine("Visual Studio: $($_.Exception.Message)")
    exit 1
}
