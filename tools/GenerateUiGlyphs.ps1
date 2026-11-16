# Generate raster glyph atlases with the OS text renderer; no font binary is embedded.
Add-Type -AssemblyName System.Drawing
foreach($spec in @(@('sans','Segoe UI'),@('roman','Georgia'))) {
    $bitmap=New-Object Drawing.Bitmap 1024,480
    $graphics=[Drawing.Graphics]::FromImage($bitmap)
    $graphics.Clear([Drawing.Color]::Transparent)
    $graphics.TextRenderingHint=[Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    $font=New-Object Drawing.Font $spec[1],46,([Drawing.FontStyle]::Regular),([Drawing.GraphicsUnit]::Pixel)
    $format=[Drawing.StringFormat]::GenericTypographic
    $metrics=New-Object 'System.Collections.Generic.List[string]'
    for($code=32;$code -le 126;$code++) {
        $index=$code-32;$x=($index%16)*64;$y=[Math]::Floor($index/16)*80
        $text=[string][char]$code
        $advance=$graphics.MeasureString($text,$font,1000,$format).Width
        if($code -eq 32){$advance=15}
        $graphics.DrawString($text,$font,[Drawing.Brushes]::White,([single]($x+2)),([single]($y+2)),$format)
        $metrics.Add("$code $x $y $advance")
    }
    $bitmap.Save((Join-Path (Get-Location) "assets/ui/$($spec[0]).png"))
    [IO.File]::WriteAllLines((Join-Path (Get-Location) "assets/ui/$($spec[0]).txt"),$metrics)
    $font.Dispose();$graphics.Dispose();$bitmap.Dispose()
}
