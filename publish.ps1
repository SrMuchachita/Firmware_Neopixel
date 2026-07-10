# publish.ps1 — Compila, sube Release a GitHub y actualiza version.json

# Leer version del CMakeLists.txt
$match = Select-String -Path "CMakeLists.txt" -Pattern 'project\(\w+ VERSION ([\d.]+)\)'
if (-not $match) { Write-Error "No se encontro VERSION en CMakeLists.txt"; exit 1 }
$version = $match.Matches[0].Groups[1].Value
Write-Host "`n>>> Version detectada: $version`n" -ForegroundColor Cyan

# Compilar
Write-Host ">>> Compilando..." -ForegroundColor Yellow
idf.py build
if ($LASTEXITCODE -ne 0) { Write-Error "Build fallo"; exit 1 }

# Crear Release en GitHub y subir firmware.bin
Write-Host "`n>>> Creando Release v$version en GitHub..." -ForegroundColor Yellow
gh release create "v$version" "build/firmware.bin#firmware.bin" `
    --title "v$version" `
    --notes "Version $version"
if ($LASTEXITCODE -ne 0) { Write-Error "No se pudo crear el Release"; exit 1 }

# Actualizar version.json
Write-Host "`n>>> Actualizando version.json..." -ForegroundColor Yellow
Set-Content -Path "version.json" -Value "{`"version`":`"$version`"}" -NoNewline

# Commit y push
git add version.json
git commit -m "v$version"
git push

Write-Host "`n>>> Listo! El ESP32 se actualizara en el proximo chequeo.`n" -ForegroundColor Green
