# Puts an emulator plugin's settings description and the languages of its text into its file, as the sections
# .asisettings and .asisettings.<lang>. They aren't loaded: every byte already in the file stays where it is, the
# section names and the section table are written anew at the end.
# With -Objcopy the module loses its debug info first: readers don't open a module above 16 MiB, and nothing loads it.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Module,
    [Parameter(Mandatory)][string]$Description,
    [Parameter(Mandatory)][string]$Text,
    [string]$Objcopy
)
$ErrorActionPreference = 'Stop'
if ($Objcopy) {
    & $Objcopy '--strip-debug' $Module.Replace('\', '/')
    if ($LASTEXITCODE) { throw "Removing the debug info failed: $Module" }
}
$file = [IO.File]::ReadAllBytes($Module)
if ($file.Length -lt 52 -or [BitConverter]::ToUInt32($file, 0) -ne 0x464c457f -or $file[4] -ne 1 -or $file[5] -ne 1) { throw "Not a 32-bit little endian ELF file: $Module" }
$tableAt = [BitConverter]::ToUInt32($file, 32)
$count = [BitConverter]::ToUInt16($file, 48)
$namesIndex = [BitConverter]::ToUInt16($file, 50)
if ([BitConverter]::ToUInt16($file, 46) -ne 40 -or !$count -or $namesIndex -ge $count) { throw "No section table: $Module" }
$namesEntry = $tableAt + $namesIndex * 40
$oldNames = New-Object byte[] ([BitConverter]::ToUInt32($file, $namesEntry + 20))
[Array]::Copy($file, [BitConverter]::ToUInt32($file, $namesEntry + 16), $oldNames, 0, $oldNames.Length)
if ([Text.Encoding]::ASCII.GetString($oldNames).Contains('.asisettings')) { throw "The settings are already in the module: $Module" }

$output = New-Object IO.MemoryStream
$output.Write($file, 0, $file.Length)
$names = New-Object IO.MemoryStream
$names.Write($oldNames, 0, $oldNames.Length)
$table = New-Object IO.MemoryStream
$table.Write($file, $tableAt, $count * 40)
$writer = New-Object IO.BinaryWriter($table)

$sections = @(, @('.asisettings', $Description)) + @(Get-ChildItem -LiteralPath $Text -Filter '*.json' -File | Sort-Object Name | ForEach-Object { , @(".asisettings.$($_.BaseName)", $_.FullName) })
foreach ($section in $sections) {
    $content = [IO.File]::ReadAllBytes($section[1])
    $name = [Text.Encoding]::ASCII.GetBytes($section[0] + "`0")
    $nameAt = [uint32]$names.Length
    $names.Write($name, 0, $name.Length)
    # name, SHT_PROGBITS, no flags, no address, offset, size, link, info, alignment, entry size
    foreach ($value in @($nameAt, 1, 0, 0, $output.Length, $content.Length, 0, 0, 1, 0)) { $writer.Write([uint32]$value) }
    $output.Write($content, 0, $content.Length)
}
$writer.Flush()

# The section names, now with the new ones
$newNames = $names.ToArray()
$table.Position = $namesIndex * 40 + 16
$writer.Write([uint32]$output.Length)
$writer.Write([uint32]$newNames.Length)
$writer.Flush()
$output.Write($newNames, 0, $newNames.Length)

while ($output.Length % 4) { $output.WriteByte(0) }
$newTableAt = [uint32]$output.Length
$newTable = $table.ToArray()
$output.Write($newTable, 0, $newTable.Length)
$result = $output.ToArray()
[Array]::Copy([BitConverter]::GetBytes($newTableAt), 0, $result, 32, 4)
[Array]::Copy([BitConverter]::GetBytes([uint16]($count + $sections.Count)), 0, $result, 48, 2)
if ($result.Length -gt 16MB) { throw "The module is larger than readers of its settings accept (16 MiB): $Module" }

$temporary = $Module + '.tmp'
try {
    [IO.File]::WriteAllBytes($temporary, $result)
    Move-Item -LiteralPath $temporary -Destination $Module -Force
} finally {
    Remove-Item -LiteralPath $temporary -Force -ErrorAction SilentlyContinue
}
Write-Host "Settings embedded ($($sections.Count) sections): $Module"
