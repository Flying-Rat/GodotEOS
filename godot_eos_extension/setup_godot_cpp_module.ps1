# Helper script to initialize or populate the godot-cpp module for GodotEOS (compacted)

# Parse the branch from .gitmodules
$gitmodulesPath = "../.gitmodules"
if (Test-Path $gitmodulesPath) {
    $gitmodulesContent = Get-Content $gitmodulesPath -Raw
    if ($gitmodulesContent -match '\[submodule "godot_eos_extension/godot-cpp"\][\s\S]*?branch\s*=\s*([^\s\n]+)') {
        $DesiredModuleBranch = $matches[1].Trim()
        Write-Host "Using branch '$DesiredModuleBranch' from .gitmodules"
    } else {
        Write-Warning "Branch not found in .gitmodules; falling back to default 'godot-4.3-stable'"
        $DesiredModuleBranch = 'godot-4.3-stable'
    }
} else {
    Write-Warning ".gitmodules not found; falling back to default 'godot-4.3-stable'"
    $DesiredModuleBranch = 'godot-4.3-stable'
}

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Error "git is not available in PATH. Cannot initialize submodules."; exit 1
}

Write-Host "Initializing git submodules for GodotEOS extension..."
git submodule update --init --recursive
if ($LASTEXITCODE -ne 0) { Write-Error "git submodule init/update failed."; exit 1 }

$cppPath = 'godot-cpp'
if (-not (Test-Path $cppPath) -or -not (Get-ChildItem -Path $cppPath -File -Recurse -ErrorAction SilentlyContinue)) {
    Write-Error "'$cppPath' missing or empty after init."; exit 1
}

Write-Host "godot-cpp initialized. Attempting branch '$DesiredModuleBranch'..."

# Attempt to switch to the desired branch if the submodule is a git repo.
if (Test-Path (Join-Path $cppPath '.git')) {
    Push-Location $cppPath
    try {
        git fetch origin --prune 2>$null
        if (git rev-parse --verify $DesiredModuleBranch 2>$null) {
            Write-Host "Checking out local branch/tag '$DesiredModuleBranch'..."
            git checkout $DesiredModuleBranch 2>$null
        } elseif (git ls-remote --heads origin $DesiredModuleBranch 2>$null | Select-String $DesiredModuleBranch) {
            Write-Host "Creating tracking branch from origin/$DesiredModuleBranch..."
            git checkout -b $DesiredModuleBranch origin/$DesiredModuleBranch 2>$null
        } elseif (git ls-remote --tags origin $DesiredModuleBranch 2>$null | Select-String $DesiredModuleBranch) {
            Write-Host "Checking out tag '$DesiredModuleBranch'..."
            git checkout $DesiredModuleBranch 2>$null
        } else {
            Write-Host "Branch/tag '$DesiredModuleBranch' not available; leaving as-is."
        }
        if ($LASTEXITCODE -ne 0) { Write-Warning "Branch/tag operation failed." }
    } finally { Pop-Location }
}
