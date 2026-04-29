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

Invoke-PythonScript (Join-Path $ScriptDir "run_case.py") $args
