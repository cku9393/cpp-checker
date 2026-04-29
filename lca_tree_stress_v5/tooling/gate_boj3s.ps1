$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

function Invoke-PythonScript {
    param(
        [string]$ScriptPath,
        [string[]]$ScriptArgs
    )

    if (Get-Command py -ErrorAction SilentlyContinue) {
        & py -3 $ScriptPath @ScriptArgs
    }
    elseif (Get-Command python -ErrorAction SilentlyContinue) {
        & python $ScriptPath @ScriptArgs
    }
    else {
        throw "Python 3 launcher not found. Install Python 3 and expose 'py' or 'python' on PATH."
    }

    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

$solver = if ($args.Count -ge 1) { $args[0] } else { ".\solve.exe" }
$outDir = if ($args.Count -ge 2) { $args[1] } else { "boj3s_out" }
$limitScale = if ($args.Count -ge 3) { $args[2] } else { "1.0" }

Invoke-PythonScript (Join-Path $ScriptDir "certify_suite.py") @(
    "--solver", $solver,
    "--preset", "suite_presets/boj_3s_hard_gate.json",
    "--out", $outDir,
    "--limit-scale", $limitScale
)
