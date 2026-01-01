# Fix-MarkdownUnicode.ps1
# Scans and fixes Unicode rendering issues in markdown files

param(
    [switch]$WhatIf
)

Write-Host "=== Markdown Unicode Fixer ===" -ForegroundColor Cyan
Write-Host ""

# Get all markdown files (excluding private-docs and build folders)
$mdFiles = Get-ChildItem -Path . -Recurse -Include "*.md" | 
    Where-Object { 
        $_.FullName -notmatch '\\private-docs\\' -and
        $_.FullName -notmatch '\\build\\' -and
        $_.FullName -notmatch '\\out\\' -and
        $_.FullName -notmatch '\\.git\\'
    }

Write-Host "Found $($mdFiles.Count) markdown files to check" -ForegroundColor Yellow
Write-Host ""

$totalFixed = 0
$filesFixed = 0

foreach ($file in $mdFiles) {
    $relativePath = $file.FullName.Replace($PWD.Path, ".").TrimStart('\')
    
    $content = Get-Content $file.FullName -Raw -Encoding UTF8
    $originalContent = $content
    $fixCount = 0
    
    # Fix box-drawing characters (Unicode 2500-257F)
    # These often display as ? on some systems
    $boxChars = @(
        @{Find = [char]0x2500; Replace = '-'},  # BOX DRAWINGS LIGHT HORIZONTAL
        @{Find = [char]0x2502; Replace = '|'},  # BOX DRAWINGS LIGHT VERTICAL
        @{Find = [char]0x250C; Replace = '+'},  # BOX DRAWINGS LIGHT DOWN AND RIGHT
        @{Find = [char]0x2510; Replace = '+'},  # BOX DRAWINGS LIGHT DOWN AND LEFT
        @{Find = [char]0x2514; Replace = '+'},  # BOX DRAWINGS LIGHT UP AND RIGHT
        @{Find = [char]0x2518; Replace = '+'},  # BOX DRAWINGS LIGHT UP AND LEFT
        @{Find = [char]0x251C; Replace = '|'},  # BOX DRAWINGS LIGHT VERTICAL AND RIGHT
        @{Find = [char]0x2524; Replace = '|'},  # BOX DRAWINGS LIGHT VERTICAL AND LEFT
        @{Find = [char]0x252C; Replace = '+'},  # BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
        @{Find = [char]0x2534; Replace = '+'},  # BOX DRAWINGS LIGHT UP AND HORIZONTAL
        @{Find = [char]0x253C; Replace = '+'}   # BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
    )
    
    foreach ($char in $boxChars) {
        if ($content.Contains($char.Find)) {
            $count = ($content.ToCharArray() | Where-Object { $_ -eq $char.Find }).Count
            $content = $content.Replace($char.Find, $char.Replace)
            $fixCount += $count
            Write-Host "  [FIX] Replaced $count box-drawing character(s) in: $relativePath" -ForegroundColor Cyan
        }
    }
    
    # Fix smart quotes
    if ($content.Contains([char]0x201C)) {
        $count = ($content.ToCharArray() | Where-Object { $_ -eq [char]0x201C }).Count
        $content = $content.Replace([char]0x201C, '"')
        $fixCount += $count
    }
    if ($content.Contains([char]0x201D)) {
        $count = ($content.ToCharArray() | Where-Object { $_ -eq [char]0x201D }).Count
        $content = $content.Replace([char]0x201D, '"')
        $fixCount += $count
    }
    if ($content.Contains([char]0x2018)) {
        $count = ($content.ToCharArray() | Where-Object { $_ -eq [char]0x2018 }).Count
        $content = $content.Replace([char]0x2018, "'")
        $fixCount += $count
    }
    if ($content.Contains([char]0x2019)) {
        $count = ($content.ToCharArray() | Where-Object { $_ -eq [char]0x2019 }).Count
        $content = $content.Replace([char]0x2019, "'")
        $fixCount += $count
    }
    
    # Fix "- ?" pattern (broken checkmarks in lists)
    if ($content -match '-\s+\?') {
        $matches = [regex]::Matches($content, '-\s+\?')
        $content = $content -replace '-\s+\?', '- ?'
        $fixCount += $matches.Count
        Write-Host "  [FIX] Replaced $($matches.Count) broken checkmark(s) '- ?' in: $relativePath" -ForegroundColor Cyan
    }
    
    # Check for multiple consecutive question marks (often indicates encoding issues)
    if ($content -match '\?{2,}') {
        $matches = [regex]::Matches($content, '\?{2,}')
        Write-Host "  [WARN] Found $($matches.Count) instance(s) of multiple '?' in: $relativePath" -ForegroundColor Yellow
        Write-Host "         This might indicate Unicode rendering issues" -ForegroundColor Gray
    }
    
    # Save if modified
    if ($content -ne $originalContent) {
        if ($WhatIf) {
            Write-Host "  [WHAT-IF] Would fix $fixCount issue(s) in: $relativePath" -ForegroundColor Magenta
        } else {
            # Ensure UTF-8 encoding without BOM
            $utf8NoBom = New-Object System.Text.UTF8Encoding $false
            [System.IO.File]::WriteAllText($file.FullName, $content, $utf8NoBom)
            Write-Host "  [FIXED] Updated $fixCount issue(s) in: $relativePath" -ForegroundColor Green
            $filesFixed++
        }
        $totalFixed += $fixCount
    }
}

Write-Host ""
Write-Host "=== Summary ===" -ForegroundColor Cyan
Write-Host "Files checked: $($mdFiles.Count)"
Write-Host "Files fixed: $filesFixed"
Write-Host "Total fixes applied: $totalFixed"

if ($WhatIf) {
    Write-Host ""
    Write-Host "Run without -WhatIf to apply fixes" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "To search for remaining question marks manually:" -ForegroundColor Yellow
Write-Host '  Select-String -Path "*.md" -Pattern "\?" -Exclude "private-docs\*"' -ForegroundColor Gray
