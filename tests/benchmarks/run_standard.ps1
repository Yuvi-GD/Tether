param(
    [string]$BinaryDirectory = "build/Release",
    [string]$OutputDirectory = "",
    [ValidateRange(0, 10000000)]
    [uint32[]]$Counts = @(1000, 10000, 100000),
    [ValidateRange(1, 20)]
    [int]$Trials = 3,
    [ValidateRange(1, 10000)]
    [int]$Frames = 120,
    [uint32]$MemoryMiB = 1024,
    [switch]$IncludeRendering,
    [switch]$OnlyLayout
)

# Explicit invocation is required. CTest and normal builds never run benchmarks.
$ErrorActionPreference = "Stop"
$runner = Join-Path (Resolve-Path -LiteralPath $BinaryDirectory) "tether_bench.exe"
if (-not (Test-Path -LiteralPath $runner)) {
    throw "Build tether_bench before running this workload matrix."
}
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path (Split-Path -Parent $runner) "report/benchmarks"
}
$runId = [DateTime]::UtcNow.ToString("yyyyMMdd-HHmmss") + "-" + [Guid]::NewGuid().ToString("N").Substring(0, 8)
$resultDirectory = New-Item -ItemType Directory -Path (Join-Path $OutputDirectory $runId) -Force
$profiles = @(
    @{ Name = "ecs-mutate"; Scenario = "ecs"; Mode = "retained"; Spawn = 0; Delete = 0; Fraction = 0 },
    @{ Name = "ecs-churn"; Scenario = "ecs"; Mode = "retained"; Spawn = 32; Delete = 32; Fraction = 0 },
    @{ Name = "ecs-fraction-25"; Scenario = "ecs"; Mode = "retained"; Spawn = 0; Delete = 0; Fraction = 0.25 }
)
if ($OnlyLayout) {
    $profiles = @(
        @{ Name = "layout-nested-resize"; Scenario = "layout"; Mode = "retained";
           Spawn = 0; Delete = 0; Fraction = 0 }
    )
}
if ($IncludeRendering -and -not $OnlyLayout) {
    $profiles += @(
        @{ Name = "render-retained"; Scenario = "render"; Mode = "retained"; Spawn = 32; Delete = 32; Fraction = 0 },
        @{ Name = "render-volatile"; Scenario = "render"; Mode = "volatile"; Spawn = 32; Delete = 32; Fraction = 0 }
    )
}

$runs = @()
$allSucceeded = $true
foreach ($count in $Counts) {
    foreach ($profile in $profiles) {
        $mutations = if ($profile.Scenario -eq "layout") { [uint32]0 }
                     elseif ($profile.Fraction) { [uint32][Math]::Ceiling($count * $profile.Fraction) }
                     else { [uint32]100 }
        $warmup = if ($profile.Scenario -eq "render") { 30 } else { 0 }
        for ($trial = 1; $trial -le $Trials; $trial++) {
            $filename = "$($profile.Name)-$count-trial$trial.json"
            $output = Join-Path $resultDirectory.FullName $filename
            & $runner --scenario $profile.Scenario --mode $profile.Mode --count $count `
                --spawn $profile.Spawn --delete $profile.Delete --mutate $mutations `
                --frames $Frames --warmup $warmup --fps 60 --seed 1 --memory-mib $MemoryMiB --output $output
            $exitCode = $LASTEXITCODE
            $status = "missing_report"
            $report = $null
            if (Test-Path -LiteralPath $output) {
                try {
                    $report = Get-Content -LiteralPath $output -Raw | ConvertFrom-Json
                    $status = $report.status
                    if ($report.metrics.measured_steps -ne @($report.samples).Count) {
                        $status = "inconsistent_samples"
                    }
                } catch {
                    $status = "invalid_report"
                }
            }
            $succeeded = $exitCode -eq 0 -and $status -eq "passed"
            $allSucceeded = $allSucceeded -and $succeeded
            $runs += [ordered]@{
                profile = $profile.Name
                requested_count = $count
                mutate_per_step = $mutations
                requested_mutations_total = [uint64]$mutations * [uint64]$Frames
                actual_mutations_total = $(if ($report -and $status -eq "passed") {
                    [uint64](($report.samples | Measure-Object -Property mutated -Sum).Sum)
                } else { $null })
                trial = $trial
                exit_code = $exitCode
                execution_status = $(if ($succeeded) { "completed" } else { "unsuccessful" })
                runner_status = $status
                report = $filename
            }
            # Publish progress after every run so interruption leaves an inventory.
            [ordered]@{ suite = $(if ($OnlyLayout) { "tether-layout-v1" } else { "tether-standard-v1" }); runs = $runs } |
                ConvertTo-Json -Depth 6 |
                Set-Content -LiteralPath (Join-Path $resultDirectory.FullName "manifest.json") -Encoding UTF8
        }
    }
}
Write-Host "Reports: $($resultDirectory.FullName)"
if (-not $allSucceeded) {
    exit 1
}
