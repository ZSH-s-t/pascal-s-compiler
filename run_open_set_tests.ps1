# open_set: run every *.pas through pascc -> gcc -> run (optional .in).
#
# Default (no flags):
#   - pascc must exit 0 (semantic OK per main.c)
#   - generated .c must compile with gcc, program must run
#   - If open_set\XX.out exists (UTF-8 text), stdout is compared to that file (OJ-style oracle).
#     No .out file => no stdout check (only pipeline). Add .out when you export expected output from OJ/TA.
#   Golden XX.c is not used unless you pass a switch below.
#
# Optional:
#   -CompareGoldenStdout   if no XX.out: compare stdout to golden XX.c run; if XX.out exists, .out wins
#   -CompareGoldenSource   compare generated .c text to golden XX.c (line endings normalized)
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File run_open_set_tests.ps1
#   powershell -ExecutionPolicy Bypass -File run_open_set_tests.ps1 -CompareGoldenStdout
#   powershell -ExecutionPolicy Bypass -File run_open_set_tests.ps1 -CompareGoldenSource

param(
    [switch]$CompareGoldenStdout,
    [switch]$CompareGoldenSource
)

$ErrorActionPreference = "Continue"
$Root = Split-Path -Parent $PSScriptRoot
$OpenSet = Join-Path $Root "open_set"
$Pascc = Join-Path $PSScriptRoot "pascc.exe"
$WorkDir = Join-Path $env:TEMP ("pascc_open_set_" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null

function Normalize-Out([string]$s) {
    if ($null -eq $s) { return "" }
    $t = $s.TrimEnd()
    return ($t -replace "`r`n", "`n" -replace "`r", "`n")
}

function Normalize-SourceText([string]$s) {
    if ($null -eq $s) { return "" }
    return ($s -replace "`r`n", "`n" -replace "`r", "`n").TrimEnd() + "`n"
}

function Get-ProgramOutput {
    param([string]$ExePath, [string]$InPath)
    if ($InPath -and (Test-Path $InPath)) {
        Get-Content -LiteralPath $InPath -Raw | & $ExePath 2>&1
    } else {
        & $ExePath 2>&1
    }
}

try {
    if (-not (Test-Path -LiteralPath $Pascc)) {
        Write-Host "ERROR: pascc.exe not found. Run build.bat first."
        exit 1
    }

    if ($CompareGoldenStdout -and $CompareGoldenSource) {
        Write-Host "ERROR: use only one of -CompareGoldenStdout or -CompareGoldenSource at a time."
        exit 1
    }

    $script:passCount = 0
    $script:outCompared = 0
    $fail = New-Object System.Collections.ArrayList

    Get-ChildItem -LiteralPath $OpenSet -Filter "*.pas" | Sort-Object Name | ForEach-Object {
        $pas = $_.FullName
        $base = $_.BaseName
        $golden = Join-Path $OpenSet ($base + ".c")
        $infile = Join-Path $OpenSet ($base + ".in")
        $hasGolden = Test-Path -LiteralPath $golden

        $genC = Join-Path $WorkDir ($base + "_gen.c")
        & $Pascc -i $pas -o $genC 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            [void]$fail.Add("$base : pascc failed (exit=$LASTEXITCODE, need semantic OK)")
            return
        }

        if ($CompareGoldenSource) {
            if (-not $hasGolden) {
                [void]$fail.Add("$base : -CompareGoldenSource but no golden $base.c")
                return
            }
            $gText = Normalize-SourceText (Get-Content -LiteralPath $genC -Raw -ErrorAction SilentlyContinue)
            $rText = Normalize-SourceText (Get-Content -LiteralPath $golden -Raw -ErrorAction SilentlyContinue)
            if ($gText -ne $rText) {
                [void]$fail.Add("$base : generated .c text != golden .c (see diff manually)")
                return
            }
        }

        $genExe = Join-Path $WorkDir ($base + "_gen.exe")
        & gcc -std=c99 -Wall -o $genExe $genC 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            [void]$fail.Add("$base : gcc on generated .c failed")
            return
        }

        $genOut = Get-ProgramOutput -ExePath $genExe -InPath $infile
        $genStr = Normalize-Out ($genOut | Out-String)

        $outFile = Join-Path $OpenSet ($base + ".out")
        if (Test-Path -LiteralPath $outFile) {
            $script:outCompared++
            $expRaw = Get-Content -LiteralPath $outFile -Raw -Encoding UTF8
            $expStr = Normalize-Out $expRaw
            if ($genStr -ne $expStr) {
                $e = ($expStr -replace "`n", "\n")
                $a = ($genStr -replace "`n", "\n")
                if ($e.Length -gt 200) { $e = $e.Substring(0, 200) + "..." }
                if ($a.Length -gt 200) { $a = $a.Substring(0, 200) + "..." }
                [void]$fail.Add("$base : stdout != $base.out (OJ-style)`n  expected: $e`n  actual:   $a")
                return
            }
        } elseif ($CompareGoldenStdout) {
            if (-not $hasGolden) {
                [void]$fail.Add("$base : -CompareGoldenStdout but no $base.out and no golden $base.c")
                return
            }
            $refExe = Join-Path $WorkDir ($base + "_ref.exe")
            & gcc -std=c99 -o $refExe $golden 2>&1 | Out-Null
            if ($LASTEXITCODE -ne 0) {
                [void]$fail.Add("$base : golden .c does not compile (gcc)")
                return
            }
            $refOut = Get-ProgramOutput -ExePath $refExe -InPath $infile
            $refStr = Normalize-Out ($refOut | Out-String)
            if ($refStr -ne $genStr) {
                $e = ($refStr -replace "`n", "\n")
                $a = ($genStr -replace "`n", "\n")
                if ($e.Length -gt 200) { $e = $e.Substring(0, 200) + "..." }
                if ($a.Length -gt 200) { $a = $a.Substring(0, 200) + "..." }
                [void]$fail.Add("$base : stdout != golden .c run`n  golden: $e`n  yours:  $a")
                return
            }
        }

        $script:passCount++
    }

    $totalPas = (Get-ChildItem -LiteralPath $OpenSet -Filter "*.pas").Count
    $mode = "pipeline (pascc exit 0 + gcc + run); if XX.out present, stdout vs .out (UTF-8)"
    if ($CompareGoldenStdout) {
        $mode = "pipeline; stdout vs XX.out if present, else vs golden .c run"
    }
    if ($CompareGoldenSource) { $mode = "pipeline + generated .c text vs golden .c" }

    Write-Host "========== open_set =========="
    Write-Host "Mode: $mode"
    Write-Host "PASS: $($script:passCount) / $totalPas"
    Write-Host "Cases with XX.out checked: $($script:outCompared)"
    Write-Host ""
    if ($fail.Count -gt 0) {
        Write-Host "---- FAIL ----"
        foreach ($x in $fail) { Write-Host $x; Write-Host "" }
        exit 1
    }
    Write-Host "ALL TESTS PASSED."
    exit 0
}
finally {
    Remove-Item -LiteralPath $WorkDir -Recurse -Force -ErrorAction SilentlyContinue
}
