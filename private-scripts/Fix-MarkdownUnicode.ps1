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
    
    # Read file as UTF-8 (with BOM detection)
    $content = [System.IO.File]::ReadAllText($file.FullName, [System.Text.Encoding]::UTF8)
    $originalContent = $content
    $fixCount = 0
    
    # Pattern 1: Replace "?? " (double question mark with space) with warning emoji
    $beforeCount = ($content.ToCharArray() | Where-Object { $_ -eq '?' }).Count
    $content = $content -replace '\?\?\s+', '?? '
    $afterCount = ($content.ToCharArray() | Where-Object { $_ -eq '?' }).Count
    if ($beforeCount -ne $afterCount) {
        $replaced = ($beforeCount - $afterCount) / 2  # Each ?? = 2 question marks
        $fixCount += $replaced
        Write-Host "  [FIX] Replaced $replaced warning indicator(s) '?? ' in: $relativePath" -ForegroundColor Cyan
    }
    
    # Pattern 2: Replace "| ? " (pipe space question mark space) with checkmark
    $pattern = '\|\s+\?\s+'
    $matches = [regex]::Matches($content, $pattern)
    if ($matches.Count -gt 0) {
        $content = $content -replace $pattern, '| ? '
        $fixCount += $matches.Count
        Write-Host "  [FIX] Replaced $($matches.Count) table checkmark(s) '| ? ' in: $relativePath" -ForegroundColor Cyan
    }
    
    # Pattern 3: Replace "- ? " (list item with checkmark) at start of line
    $pattern = '^(\s*)-\s+\?\s+'
    $matches = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::Multiline)
    if ($matches.Count -gt 0) {
        $content = $content -replace $pattern, '$1- ? '
        $fixCount += $matches.Count
        Write-Host "  [FIX] Replaced $($matches.Count) list checkmark(s) '- ? ' in: $relativePath" -ForegroundColor Cyan
    }
    
    # Pattern 4: Replace "### ? " (header with checkmark) at start of line  
    $pattern = '^(#{1,6}\s+)\?\s+'
    $matches = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::Multiline)
    if ($matches.Count -gt 0) {
        $content = $content -replace $pattern, '$1? '
        $fixCount += $matches.Count
        Write-Host "  [FIX] Replaced $($matches.Count) header checkmark(s) '# ? ' in: $relativePath" -ForegroundColor Cyan
    }
    
    # Pattern 5: Replace standalone "?" in numbered lists (e.g., "1. ? All")
    $pattern = '^(\s*\d+\.\s+)\?\s+'
    $matches = [regex]::Matches($content, $pattern, [System.Text.RegularExpressions.RegexOptions]::Multiline)
    if ($matches.Count -gt 0) {
        $content = $content -replace $pattern, '$1? '
        $fixCount += $matches.Count
        Write-Host "  [FIX] Replaced $($matches.Count) numbered list checkmark(s) in: $relativePath" -ForegroundColor Cyan
    }
    
    # Pattern 6: Replace "**Status:** ?" patterns with cross mark
    $pattern = '(\*\*Status:\*\*\s+)\?\s+Not\s+Implemented'
    $matches = [regex]::Matches($content, $pattern)
    if ($matches.Count -gt 0) {
        $content = $content -replace $pattern, '$1? Not Implemented'
        $fixCount += $matches.Count
        Write-Host "  [FIX] Replaced $($matches.Count) status cross(es) in: $relativePath" -ForegroundColor Cyan
    }
    
    # Save if modified
    if ($content -ne $originalContent) {
        if ($WhatIf) {
            Write-Host "  [WHAT-IF] Would fix $fixCount issue(s) in: $relativePath" -ForegroundColor Magenta
        } else {
            # Write with UTF-8 BOM for emoji support
            $utf8WithBom = New-Object System.Text.UTF8Encoding $true
            [System.IO.File]::WriteAllText($file.FullName, $content, $utf8WithBom)
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
Write-Host "Emoji replacements:" -ForegroundColor Yellow
Write-Host "  ? = Success/Pass/Implemented" -ForegroundColor Green
Write-Host "  ??  = Warning/Partial/Issues" -ForegroundColor Yellow  
Write-Host "  ? = Failed/Not Implemented" -ForegroundColor Red
