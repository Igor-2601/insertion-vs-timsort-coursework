param([string]$Cxx = 'clang++', [string]$Results = 'results_new')
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    New-Item -ItemType Directory -Path 'build' -Force | Out-Null
    & $Cxx -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include tests/test_sorts.cpp -o build/sort_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed' }
    & ./build/sort_tests.exe
    if ($LASTEXITCODE -ne 0) { throw 'Correctness tests failed' }
    & $Cxx -std=c++17 -O2 -Wall -Wextra -Wpedantic -I include src/benchmark.cpp -o build/sort_benchmark.exe
    if ($LASTEXITCODE -ne 0) { throw 'Benchmark compilation failed' }
    & ./build/sort_benchmark.exe $Results
    if ($LASTEXITCODE -ne 0) { throw 'Benchmark failed' }
} finally {
    Pop-Location
}
