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
$outDir = if ($args.Count -ge 2) { $args[1] } else { "hunt_out" }
$sizes = if ($args.Count -ge 3) { $args[2] } else { "12000,24000,48000,99999" }
$seeds = if ($args.Count -ge 4) { $args[3] } else { "1,2,3" }
$timeout = if ($args.Count -ge 5) { $args[4] } else { "8.0" }

Invoke-PythonScript (Join-Path $ScriptDir "hunt_hardest.py") @(
    "--solver", $solver,
    "--out", $outDir,
    "--sizes", $sizes,
    "--seeds", $seeds,
    "--timeout", $timeout
)
