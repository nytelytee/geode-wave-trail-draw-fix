param (
  [switch]$cleanup = $false,
  [switch]$reconfigure = $false,
  [switch]$rungame = $false,
  [string][ValidateSet('msvc', 'clang', 'MSVC', 'Clang')]$compiler = "clang",
  [string]$mode = "RelWithDebInfo"
)

if ($compiler -eq "Clang") { $compiler="clang" }
if ($compiler -eq "MSVC") { $compiler="msvc" }

if ($cleanup) {
  Write-Output "Deleting all build folders..."
  $sw = [Diagnostics.Stopwatch]::StartNew()
  if (Test-Path -Path "./build/") {
    Remove-Item -Path "./build/" -Force -Recurse
  }
  $sw.Stop()
  Write-Output "Took $($sw.Elapsed.TotalSeconds)s."
  exit
}

if ($reconfigure -And (Test-Path -Path "./build/$compiler/$mode")) {
  Write-Output "Deleting build/$compiler/$mode..."
  $sw = [Diagnostics.Stopwatch]::StartNew()
	Remove-Item -Path "./build/$compiler/$mode" -Force -Recurse
  $sw.Stop()
  Write-Output "Took $($sw.Elapsed.TotalSeconds)s."
}

if ($compiler -eq "msvc" -And -Not (Test-Path -Path "./build/$compiler/$mode")) {
  Write-Output "Configuring build/$compiler/$mode..."
  $sw = [Diagnostics.Stopwatch]::StartNew()
	cmake -B "./build/$compiler/$mode" -A x64 -DCMAKE_BUILD_TYPE="$mode"
  $sw.Stop()
  Write-Output "Took $($sw.Elapsed.TotalSeconds)s."
} elseif ($compiler -eq "clang" -And -Not (Test-Path -Path "./build/$compiler/$mode")) {
  Write-Output "Configuring build/$compiler/$mode..."
  $sw = [Diagnostics.Stopwatch]::StartNew()
  cmake -B "./build/$compiler/$mode" -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE="$mode"
  $sw.Stop()
  Write-Output "Took $($sw.Elapsed.TotalSeconds)s."
}

Write-Output "Building build/$compiler/$mode..."
$sw = [Diagnostics.Stopwatch]::StartNew()
cmake --build "./build/$compiler/$mode" --config $mode
$built = $?
$sw.Stop()
Write-Output "Took $($sw.Elapsed.TotalSeconds)s."

if ($built -And $rungame) {
	taskkill /IM GeometryDash.exe 2>$null
	steam steam://rungameid/322170
}
