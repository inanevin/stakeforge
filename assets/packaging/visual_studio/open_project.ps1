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
    [Parameter(Mandatory = $true)][string]$SolutionPath,
    [ValidateSet('Open', 'Attach', 'Detach')][string]$Action = 'Open',
    [uint32]$IdeProcessId = 0,
    [uint64]$IdeCreationTime = 0,
    [uint32]$TargetProcessId = 0
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

function Test-VisualStudioDebuggedProcess {
    param($Debugger, [uint32]$ProcessId)

    $processes = Invoke-VisualStudio { $Debugger.DebuggedProcesses }

    foreach ($process in $processes) {
        $debuggedId = Invoke-VisualStudio { $process.ProcessID }

        if ($debuggedId -eq $ProcessId) {
            return $true
        }
    }

    return $false
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

        public static object[] GetInstances(string programId, uint processId)
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

                        bool matches = processId == 0
                            ? name.StartsWith("!" + programId + ":", StringComparison.OrdinalIgnoreCase)
                            : string.Equals(name, "!" + programId + ":" + processId, StringComparison.OrdinalIgnoreCase);

                        if (matches)
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
    if ($Action -ne 'Open') {
        if ($IdeProcessId -eq 0 -or $IdeCreationTime -eq 0 -or $TargetProcessId -eq 0) {
            throw 'Debugger control requires the IDE identity and target process.'
        }

        $ideProcess = Get-Process -Id $IdeProcessId -ErrorAction SilentlyContinue

        if ($null -eq $ideProcess) {
            if ($Action -eq 'Attach') {
                throw 'Visual Studio closed before the debugger could attach.'
            }

            Write-Output 'Visual Studio has closed; its debugger session has ended.'
            exit 0
        }

        if ($ideProcess.HasExited -or $ideProcess.StartTime.ToFileTimeUtc() -ne $IdeCreationTime) {
            if ($Action -eq 'Attach') {
                throw 'The Visual Studio session expired before the debugger could attach.'
            }

            Write-Output 'The Visual Studio session has expired; its debugger session has ended.'
            exit 0
        }
    }

    $solution = if ($Action -eq 'Detach') { [IO.Path]::GetFullPath($SolutionPath) } else { (Resolve-Path -LiteralPath $SolutionPath).Path }
    $settingsPath = Join-Path $PSScriptRoot '../../stakeforge_sdk/build_settings.json'
    $settings = Get-Content -LiteralPath $settingsPath -Raw | ConvertFrom-Json
    $generator = [regex]::Match($settings.generator, '^Visual Studio (\d+) ')

    if (-not $generator.Success) {
        throw 'The SDK does not specify a Visual Studio generator.'
    }

    $programId = 'VisualStudio.DTE.' + $generator.Groups[1].Value + '.0'
    $dte = $null

    $instanceProcessId = if ($Action -ne 'Open') { $IdeProcessId } else { 0 }

    foreach ($candidate in [Stakeforge.VisualStudioAutomation]::GetInstances($programId, $instanceProcessId)) {
        if ($Action -eq 'Detach') {
            $dte = $candidate
            break
        }

        $candidateSolution = Invoke-VisualStudio { $candidate.Solution.FullName }

        if ([string]::Equals($candidateSolution, $solution, [StringComparison]::OrdinalIgnoreCase)) {
            $dte = $candidate
            break
        }
    }

    if ($null -eq $dte) {
        if ($Action -ne 'Open') {
            throw 'The tracked Visual Studio instance no longer exposes the game plugin solution.'
        }

        $automationType = [Type]::GetTypeFromProgID($programId, $true)
        $dte = Invoke-VisualStudio { [Activator]::CreateInstance($automationType) }
        Invoke-VisualStudio { $dte.UserControl = $true }
        Invoke-VisualStudio { $dte.Solution.Open($solution) }
    }

    $ideExecutable = Invoke-VisualStudio { $dte.FullName }
    $assemblyPath = Join-Path (Split-Path -Parent $ideExecutable) 'PublicAssemblies/envdte.dll'
    $automationAssembly = [Reflection.Assembly]::LoadFrom($assemblyPath)

    if ($Action -eq 'Detach') {
        $debugger = Invoke-VisualStudio { $dte.Debugger }
        $processes = Invoke-VisualStudio { $debugger.DebuggedProcesses }
        $detachMethod = $automationAssembly.GetType('EnvDTE.Process', $true).GetMethod('Detach')

        foreach ($debuggedProcess in $processes) {
            $debuggedId = Invoke-VisualStudio { $debuggedProcess.ProcessID }

            if ($debuggedId -eq $TargetProcessId) {
                Invoke-VisualStudio { [void]$detachMethod.Invoke($debuggedProcess, @($false)) }
                break
            }
        }

        while (Test-VisualStudioDebuggedProcess $debugger $TargetProcessId) {
            if ([DateTime]::UtcNow -ge $deadline) {
                throw 'Timed out waiting for the Visual Studio debugger to detach.'
            }

            Start-Sleep -Milliseconds 100
        }

        Write-Output "Visual Studio process $IdeProcessId stopped debugging Stakeforge process $TargetProcessId."
        exit 0
    }

    if ($Action -eq 'Attach') {
        $debuggerAssemblyPath = Join-Path (Split-Path -Parent $ideExecutable) 'PublicAssemblies/EnvDTE80.dll'
        $debuggerAssembly = [Reflection.Assembly]::LoadFrom($debuggerAssemblyPath)
        $attachMethod = $debuggerAssembly.GetType('EnvDTE80.Process2', $true).GetMethod('Attach2')
        $debugger = Invoke-VisualStudio { $dte.Debugger }
        $processes = Invoke-VisualStudio { $debugger.LocalProcesses }
        $target = $null

        foreach ($candidateProcess in $processes) {
            $candidateId = Invoke-VisualStudio { $candidateProcess.ProcessID }

            if ($candidateId -eq $TargetProcessId) {
                $target = $candidateProcess
                break
            }
        }

        if ($null -eq $target) {
            throw 'The Stakeforge editor process is no longer available.'
        }

        $attached = Test-VisualStudioDebuggedProcess $debugger $TargetProcessId

        if (-not $attached) {
            Invoke-VisualStudio { [void]$attachMethod.Invoke($target, @('native')) }
        }

        while (-not (Test-VisualStudioDebuggedProcess $debugger $TargetProcessId)) {
            if ([DateTime]::UtcNow -ge $deadline) {
                throw 'Timed out waiting for the Visual Studio debugger to attach.'
            }

            Start-Sleep -Milliseconds 100
        }

        Write-Output "Visual Studio process $IdeProcessId is debugging Stakeforge process $TargetProcessId."
        exit 0
    }

    $mainWindowProperty = $automationAssembly.GetType('EnvDTE.DTE', $true).GetProperty('MainWindow')
    $windowProperty = $automationAssembly.GetType('EnvDTE.Window', $true).GetProperty('HWnd')

    while ($true) {
        $window = Invoke-VisualStudio {
            $mainWindow = $mainWindowProperty.GetValue($dte)

            if ($null -eq $mainWindow) {
                return [IntPtr]::Zero
            }

            $mainWindow.Visible = $true
            [void]$mainWindow.Activate()
            return [IntPtr]$windowProperty.GetValue($mainWindow)
        }

        if ($window -ne [IntPtr]::Zero) {
            break
        }

        if ([DateTime]::UtcNow -ge $deadline) {
            throw 'Timed out waiting for the Visual Studio main window.'
        }

        Start-Sleep -Milliseconds 250
    }

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
