@echo off
echo Checking out project and filters file
p4 edit ImageIO.vcxproj
p4 edit ImageIO.vcxproj.filters
p4 edit ImageIO.orbis.vcxproj
p4 edit ImageIO.orbis.vcxproj.filters
echo Regenerating
..\..\..\..\..\..\shared_tools\python\27\python.exe ..\..\tools\ProjectFileGenerator\ProjectFileGenerator.py -i ImageIO.ccpproj
pause