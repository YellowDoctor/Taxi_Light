# =====================================================================
#  Скрипт локального OTA-обновления Такси Шашки (ESP32)
#  Работает на любой Windows без установки дополнительных программ.
# =====================================================================

$ErrorActionPreference = "SilentlyContinue"
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8

# Определение папки со скриптом
$scriptDir = if ($PSScriptRoot) { $PSScriptRoot } else { Split-Path -Parent $MyInvocation.MyCommand.Definition }
if ([string]::IsNullOrEmpty($scriptDir)) {
    $scriptDir = (Get-Location).Path
}

$binName = "firmware.bin"
$binPath = Join-Path $scriptDir $binName

Write-Host ""
Write-Host "======================================================================" -ForegroundColor Cyan
Write-Host "                ОБНОВЛЕНИЕ ТАКСИ ШАШКИ ДО ВЕРСИИ 1.3.7                " -ForegroundColor Cyan
Write-Host "======================================================================" -ForegroundColor Cyan
Write-Host ""

# Проверка наличия firmware.bin
if (!(Test-Path $binPath)) {
    Write-Host "[!] Файл $binName не найден в текущей папке!" -ForegroundColor Yellow
    Write-Host "[*] Скачиваем последнюю версию 1.3.7 с GitHub..." -ForegroundColor Cyan
    try {
        $githubUrl = "https://raw.githubusercontent.com/YellowDoctor/Taxi_Light/main/firmware.bin"
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $githubUrl -OutFile $binPath -UseBasicParsing
        Write-Host "[OK] Файл $binName успешно скачан с GitHub!" -ForegroundColor Green
    } catch {
        Write-Host "[ERROR] Не удалось скачать файл: $($_.Exception.Message)" -ForegroundColor Red
        Write-Host "Пожалуйста, положите файл firmware.bin в эту папку и запустите снова." -ForegroundColor Yellow
        Read-Host "Нажмите Enter для выхода..."
        exit 1
    }
}

$fileLength = (Get-Item $binPath).Length
$fileSizeMB = [math]::Round($fileLength / 1MB, 2)
Write-Host "[+] Найден файл прошивки: $binName ($fileSizeMB МБ)" -ForegroundColor Green

# Определение активного IP адреса
$ips = @()
$defaultRoute = Get-NetRoute -DestinationPrefix '0.0.0.0/0' -ErrorAction SilentlyContinue | 
    Sort-Object RouteMetric | 
    Select-Object -First 1

$primaryIp = $null
if ($defaultRoute) {
    $primaryIp = (Get-NetIPAddress -InterfaceIndex $defaultRoute.InterfaceIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue).IPAddress
}

# Дополнительный поиск всех доступных локальных IPv4
$allIps = [System.Net.Dns]::GetHostAddresses([System.Net.Dns]::GetHostName()) | 
    Where-Object { 
        $_.AddressFamily -eq [System.Net.Sockets.AddressFamily]::InterNetwork -and 
        !$_.IPAddressToString.StartsWith("127.") -and 
        !$_.IPAddressToString.StartsWith("169.254.") 
    } | ForEach-Object { $_.IPAddressToString }

if ($primaryIp -and ($allIps -contains $primaryIp)) {
    $ips += $primaryIp
}
foreach ($ip in $allIps) {
    if ($ips -notcontains $ip) { $ips += $ip }
}

if ($ips.Count -eq 0) {
    $ips += "127.0.0.1"
}

$activeIp = $ips[0]
$port = 8080
$updateUrl = "http://${activeIp}:${port}/${binName}"

# Автоматическое копирование в буфер обмена
try {
    Set-Clipboard -Value $updateUrl -ErrorAction SilentlyContinue
    $copiedText = " (скопировано в буфер обмена!)"
} catch {
    $copiedText = ""
}

Write-Host ""
Write-Host "┌────────────────────────────────────────────────────────────────────┐" -ForegroundColor Yellow
Write-Host "│  ССЫЛКА ДЛЯ ОБНОВЛЕНИЯ В ШАШКЕ:                                    │" -ForegroundColor Yellow
Write-Host "│  $updateUrl$copiedText" -ForegroundColor White
Write-Host "└────────────────────────────────────────────────────────────────────┘" -ForegroundColor Yellow

if ($ips.Count -gt 1) {
    Write-Host "Другие доступные сетевые адреса этого компьютера:" -ForegroundColor DarkGray
    for ($i = 1; $i -lt $ips.Count; $i++) {
        Write-Host "  -> http://$($ips[$i]):${port}/${binName}" -ForegroundColor DarkGray
    }
}

Write-Host ""
Write-Host "Инструкция для обновления:" -ForegroundColor Cyan
Write-Host " 1. Откройте в браузере веб-интерфейс шашки:"
Write-Host "    http://192.168.4.1 (если подключены к Wi-Fi сети шашки TaxiLight_AP)"
Write-Host "    или http://taxilight.local (в домашней сети Wi-Fi)"
Write-Host " 2. В веб-интерфейсе перейдите в Настройки / Разработчик (/dev)"
Write-Host " 3. В поле «OTA по URL» вставьте ссылку выше и нажмите «Обновить»"
Write-Host ""
Write-Host "[•] Ожидание подключения шашки..." -ForegroundColor Yellow

# Запуск TCP сервера на порту 8080 (без требования прав администратора)
$endpoint = New-Object System.Net.IPEndPoint ([System.Net.IPAddress]::Any, $port)
$tcpListener = New-Object System.Net.Sockets.TcpListener $endpoint

try {
    $tcpListener.Start()
} catch {
    Write-Host "[ERROR] Не удалось открыть порт ${port}: $($_.Exception.Message)" -ForegroundColor Red
    Read-Host "Нажмите Enter для выхода..."
    exit 1
}

$updateCompleted = $false

while (!$updateCompleted) {
    if (!$tcpListener.Pending()) {
        Start-Sleep -Milliseconds 200
        continue
    }

    $client = $tcpListener.AcceptTcpClient()
    $remoteIp = $client.Client.RemoteEndPoint.ToString()
    $stream = $client.GetStream()
    $reader = New-Object System.IO.StreamReader($stream)

    # Чтение первой строки HTTP запроса
    $requestLine = $reader.ReadLine()
    if ([string]::IsNullOrEmpty($requestLine)) {
        $client.Close()
        continue
    }

    # Пропуск заголовков
    while ($true) {
        $headerLine = $reader.ReadLine()
        if ([string]::IsNullOrEmpty($headerLine)) { break }
    }

    # Проверка запроса
    if ($requestLine -like "*GET*") {
        Write-Host "`n[+] Шашка подключилась ($remoteIp)!" -ForegroundColor Green
        Write-Host "[*] Передача файла прошивки..." -ForegroundColor Cyan

        $headers = "HTTP/1.1 200 OK`r`n" +
                   "Content-Length: $fileLength`r`n" +
                   "Content-Type: application/octet-stream`r`n" +
                   "Connection: close`r`n`r`n"
        $headerBytes = [System.Text.Encoding]::ASCII.GetBytes($headers)
        $stream.Write($headerBytes, 0, $headerBytes.Length)

        $fileStream = [System.IO.File]::OpenRead($binPath)
        $buffer = New-Object byte[] 65536
        $sent = 0

        while (($bytesRead = $fileStream.Read($buffer, 0, $buffer.Length)) -gt 0) {
            $stream.Write($buffer, 0, $bytesRead)
            $sent += $bytesRead
            $pct = [math]::Round(($sent / $fileLength) * 100)
            Write-Host "`r[*] Прогресс: $pct% ($([math]::Round($sent/1MB, 2)) / $fileSizeMB МБ)" -NoNewline -ForegroundColor Cyan
        }
        $fileStream.Close()
        $stream.Flush()
        $client.Close()

        Write-Host "`n[OK] Файл прошивки успешно передан на шашку!" -ForegroundColor Green
        Write-Host "[*] Шашка прошивает flash-память и перезагружается..." -ForegroundColor Yellow
        $updateCompleted = $true
    } else {
        $client.Close()
    }
}

$tcpListener.Stop()

# Ожидание завершения перезагрузки шашки
Write-Host ""
for ($i = 15; $i -gt 0; $i--) {
    Write-Host "`rОжидание перезагрузки шашки: $i сек... " -NoNewline -ForegroundColor Yellow
    Start-Sleep -Seconds 1
}

Write-Host "`r======================================================================" -ForegroundColor Green
Write-Host "       ОБНОВЛЕНИЕ УСПЕШНО ЗАВЕРШЕНО! ТЕПЕРЬ ВЕРСИЯ 1.3.7!             " -ForegroundColor Green
Write-Host "======================================================================" -ForegroundColor Green
Write-Host ""
Write-Host "Что нового появилось в шашке:" -ForegroundColor White
Write-Host " • Кнопка «Обзор...» в меню для выбора файла прошивки прямо из браузера" -ForegroundColor White
Write-Host " • Кнопка «Обновить» в шапке (автопроверка релизов с GitHub)" -ForegroundColor White
Write-Host " • Поддержка защищённых ссылок HTTPS" -ForegroundColor White
Write-Host " • Режим экономии батареи (шашка больше не садится за ночь)" -ForegroundColor White
Write-Host ""

# Очистка файлов после обновления
Write-Host ""
$ans = Read-Host "Удалить файлы обновления из этой папки? (y - удалить / Enter - оставить)"
if ($ans -eq "y" -or $ans -eq "д" -or $ans -eq "yes") {
    Write-Host "[*] Удаление файлов обновления..." -ForegroundColor DarkGray
    Remove-Item $binPath -Force -ErrorAction SilentlyContinue
    $batPath = Join-Path $scriptDir "update_shashka.bat"
    $psPath  = Join-Path $scriptDir "update_shashka.ps1"
    Start-Process cmd.exe -ArgumentList "/c timeout /t 2 >nul & del `"$psPath`" `"$batPath`"" -WindowStyle Hidden
    Write-Host "[OK] Файлы обновления удалены." -ForegroundColor Green
}

Write-Host "Окно закроется через 3 секунды..." -ForegroundColor DarkGray
Start-Sleep -Seconds 3
exit 0
