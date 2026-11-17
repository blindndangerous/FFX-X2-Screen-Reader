param(
    [Parameter(Mandatory = $true)][string]$TestScript,
    [Parameter(ValueFromRemainingArguments = $true)][string[]]$TestArguments
)
$ErrorActionPreference = 'Stop'
# Children inherit this mode, including loader failures before fixture main.
Add-Type @'
using System.Runtime.InteropServices;
public static class FixtureErrorMode {
    [DllImport("kernel32.dll")]
    public static extern uint SetErrorMode(uint mode);
}
'@
[void][FixtureErrorMode]::SetErrorMode(0x0001 -bor 0x0002) # SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX
$env:FFX_TEST_NO_DIALOGS = '1'
& $TestScript @TestArguments
exit $LASTEXITCODE
