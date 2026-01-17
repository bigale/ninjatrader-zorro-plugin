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
    
    # **NEW: Fix status indicators (checkmarks and warnings)**
    # These are the most common patterns in our markdown files:
    
    # Pattern 1: "?? " or "??" at start of line or after pipe = WARNING (??)
    # Replace with ?? emoji (will render correctly)
    $pattern1 = '(\|\s*)\?\?\s+'  # Table cells: | ?? 
    if ($content -match $pattern1) {
        $matches = [regex]::Matches($content, $pattern1)
        $content = $content -replace $pattern1, '$1?? '
        $fixCount += $matches.Count
        Write-Host "  [FIX] Replaced $($matches.Count) warning indicator(s) '?? ' with '??' in: $relativePath" -ForegroundColor Cyan
    }
    
    # Pattern 2: "?? " at start of line (headers, list items)
    $pattern2 = '^(\s*)(#{1,6}\s+)?\?\?\s+'
    if ($content -match $pattern2) {
        $matches = [regex]::Matches($content, $pattern2, [System.Text.RegularExpressions.RegexOptions]::Multiline)
        $content = $content -replace $pattern2, '$1$2?? '
        $fixCount += $matches.Count
        Write-Host "  [FIX] Replaced $($matches.Count) heading/list warning(s) '?? ' with '??' in: $relativePath" -ForegroundColor Cyan
    }
    
    # Pattern 3: Single "? " in specific contexts = CHECKMARK (?)
    # Only replace in contexts where it's clearly a status indicator
    
    # 3a: Table cells with single "? "
    $pattern3a = '(\|\s*)\?\s+([A-Z])'  # | ? PASS or | ? Yes
    if ($content -match $pattern3a) {
        $matches = [regex]::Matches($content, $pattern3a)
        $content = $content -replace $pattern3a, '$1? $2'
        $fixCount += $matches.Count
        Write-Host "  [FIX] Replaced $($matches.Count) checkmark(s) '? ' with '?' in: $relativePath" -ForegroundColor Cyan
    }
    
    # 3b: List items starting with "? " (status indicators)
    $pattern3b = '^(\s*-\s+)\?\s+'
    if ($content -match $pattern3b) {
        $matches = [regex]::Matches($content, $pattern3b, [System.Text.RegularExpressions.RegexOptions]::Multiline)
        $content = $content -replace $pattern3b, '$1? '
        $fixCount += $matches.Count
        Write-Host "  [FIX] Replaced $($matches.Count) list checkmark(s) '- ? ' with '- ?' in: $relativePath" -ForegroundColor Cyan
    }
    
    # 3c: Headers with "? " prefix
    $pattern3c = '^(#{1,6}\s+)\?\s+'
    if ($content -match $pattern3c) {
        $matches = [regex]::Matches($content, $pattern3c, [System.Text.RegularExpressions.RegexOptions]::Multiline)
        $content = $content -replace $pattern3c, '$1? '
        $fixCount += $matches.Count
        Write-Host "  [FIX] Replaced $($matches.Count) header checkmark(s) '# ? ' with '# ?' in: $relativePath" -ForegroundColor Cyan
    }
    
    # Pattern 4: Status words followed by single "?" 
    # Example: "Status: ? Not Implemented" or "Impact: ? LOW"
    $pattern4 = '(Status|Implemented\?|Compliant\?|Required\?):\s+\?\s+'
    if ($content -match $pattern4) {
        $matches = [regex]::Matches($content, $pattern4)
        $content = $content -replace $pattern4, '$1: ? '
        $fixCount += $matches.Count
        Write-Host "  [FIX] Replaced $($matches.Count) negative status '?' with '?' in: $relativePath" -ForegroundColor Cyan
    }
    
    # Pattern 5: "??" in middle of sentence = keep as warning
    $pattern5 = '\s\?\?\s'
    if ($content -match $pattern5) {
        $matches = [regex]::Matches($content, $pattern5)
        $content = $content -replace $pattern5, ' ?? '
        $fixCount += $matches.Count
        Write-Host "  [FIX] Replaced $($matches.Count) inline warning(s) '??' with '??' in: $relativePath" -ForegroundColor Cyan
    }
    
    # LAST: Check for remaining question marks that might need attention
    # (but don't auto-fix these - they might be legitimate)
    $remainingQuestions = [regex]::Matches($content, '\?')
    if ($remainingQuestions.Count -gt 0) {
        # Filter out legitimate uses (in URLs, code blocks, actual questions)
        $suspiciousCount = 0
        foreach ($match in $remainingQuestions) {
            $context = $content.Substring([Math]::Max(0, $match.Index - 10), [Math]::Min(20, $content.Length - $match.Index + 10))
            # Skip if in URL, code block, or looks like actual question
            if ($context -notmatch '(http|```|SELECT|WHERE|\w\?$)') {
                $suspiciousCount++
            }
        }
        
        if ($suspiciousCount -gt 0) {
            Write-Host "  [INFO] $suspiciousCount potentially suspicious '?' characters remain in: $relativePath" -ForegroundColor Gray
        }
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
Write-Host "Emoji replacements:" -ForegroundColor Yellow
Write-Host "  ? = Success/Pass/Implemented" -ForegroundColor Green
Write-Host "  ??  = Warning/Partial/Issues" -ForegroundColor Yellow  
Write-Host "  ? = Failed/Not Implemented" -ForegroundColor Red
