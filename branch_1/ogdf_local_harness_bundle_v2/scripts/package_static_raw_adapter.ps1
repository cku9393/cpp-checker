$ErrorActionPreference = 'Stop'

$root = Split-Path $PSScriptRoot -Parent
$bundle = Join-Path $root 'artifacts_20260306_static_raw_adapter_bundle'
$zip = Join-Path $root 'artifacts_20260306_static_raw_adapter_bundle.zip'

if (Test-Path $bundle) {
    Remove-Item -Recurse -Force $bundle
}
if (Test-Path $zip) {
    Remove-Item -Force $zip
}

New-Item -ItemType Directory -Force -Path $bundle | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $bundle 'include\harness') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $bundle 'src') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $bundle 'docs') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $bundle 'verify\dumps') | Out-Null

$copies = @(
    @{ From = (Join-Path $root 'CMakeLists.txt'); To = (Join-Path $bundle 'CMakeLists.txt') }
    @{ From = (Join-Path $root 'include\harness\project_static_adapter.hpp'); To = (Join-Path $bundle 'include\harness\project_static_adapter.hpp') }
    @{ From = (Join-Path $root 'src\project_static_adapter.cpp'); To = (Join-Path $bundle 'src\project_static_adapter.cpp') }
    @{ From = (Join-Path $root 'src\project_hook_shims.cpp'); To = (Join-Path $bundle 'src\project_hook_shims.cpp') }
    @{ From = (Join-Path $root 'src\project_hooks_real.cpp'); To = (Join-Path $bundle 'src\project_hooks_real.cpp') }
    @{ From = (Join-Path $root 'docs\static_hook_search_report_ko.md'); To = (Join-Path $bundle 'docs\static_hook_search_report_ko.md') }
    @{ From = (Join-Path $root 'docs\static_adapter_design_ko.md'); To = (Join-Path $bundle 'docs\static_adapter_design_ko.md') }
    @{ From = (Join-Path $root 'artifacts_20260306_raw_adapter_verify\RESULT_ko.md'); To = (Join-Path $bundle 'verify\RESULT_ko.md') }
    @{ From = (Join-Path $root 'artifacts_20260306_raw_adapter_verify\harness-build-static-raw-configure.log'); To = (Join-Path $bundle 'verify\harness-build-static-raw-configure.log') }
    @{ From = (Join-Path $root 'artifacts_20260306_raw_adapter_verify\harness-build-static-raw-build.log'); To = (Join-Path $bundle 'verify\harness-build-static-raw-build.log') }
    @{ From = (Join-Path $root 'artifacts_20260306_raw_adapter_verify\dumps\MINI_MATERIALIZE_FAIL_seed1_tc0.txt'); To = (Join-Path $bundle 'verify\dumps\MINI_MATERIALIZE_FAIL_seed1_tc0.txt') }
)

foreach ($entry in $copies) {
    Copy-Item -LiteralPath $entry.From -Destination $entry.To
}

$readme = @'
# static raw adapter bundle

포함 내용:
- raw adapter 관련 소스 변경본
- static hook search / adapter design 문서
- hooks ON build configure/build 로그
- static/manual-only 재실행 결과 번들

현재 단계:
- RAW_VALIDATE_FAIL 탈출 완료
- 최신 실패 단계는 MINI_MATERIALIZE_FAIL
'@

Set-Content -LiteralPath (Join-Path $bundle 'README_ko.md') -Value $readme -Encoding utf8

Compress-Archive -Path (Join-Path $bundle '*') -DestinationPath $zip -CompressionLevel Optimal

Write-Output "BUNDLE=$bundle"
Write-Output "ZIP=$zip"
