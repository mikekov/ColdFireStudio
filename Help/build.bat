rem Building Studio.chm help file

"C:\Program Files (x86)\HTML Help Workshop\hhc.exe" help.hhp

if %ERRORLEVEL%==1 (
exit /b 0
) else (
echo hhc failed with errorcode %ERRORLEVEL%
exit /b 1
)
