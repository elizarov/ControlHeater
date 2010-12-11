@echo off
mkdir Dist 2>nul
mkdir Download 2>nul
del /Q/S Dist\* 1>nul
for %%a in (*.htm *.css htaccess.txt email.txt) do copy %%a Dist\ >nul
"C:\Program Files\EZWebLynx\csfs" Dist Download\data.cds
tftp -i 192.168.7.22 put Download\data.cds image.cds
