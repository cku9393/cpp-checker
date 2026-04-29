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
$outDir = if ($args.Count -ge 2) { $args[1] } else { "bench_out" }
$sizes = if ($args.Count -ge 3) { $args[2] } else { "9999,19999,39999,79999,99999" }
$seeds = if ($args.Count -ge 4) { $args[3] } else { "1" }
$timeout = if ($args.Count -ge 5) { $args[4] } else { "" }
$shuffleLabels = if ($args.Count -ge 6) { $args[5] } else { "1" }
$shuffleQueries = if ($args.Count -ge 7) { $args[6] } else { "1" }
$modes = if ($args.Count -ge 8) { $args[7] } else { "" }

$scriptArgs = @(
    "--solver", $solver,
    "--out", $outDir,
    "--sizes", $sizes,
    "--seeds", $seeds
)

if ($modes -ne "") {
    $scriptArgs += @("--modes", $modes)
}
if ($shuffleLabels -eq "1") {
    $scriptArgs += "--shuffle-labels"
}
if ($shuffleQueries -eq "1") {
    $scriptArgs += "--shuffle-queries"
}
if ($timeout -ne "") {
    $scriptArgs += @("--timeout", $timeout)
}

Invoke-PythonScript (Join-Path $ScriptDir "bench_report.py") $scriptArgs
Write-Host ""
Write-Host "[bench] summary table: $outDir/bench_summary.md"
