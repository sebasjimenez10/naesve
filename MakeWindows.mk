.phony: make movefiles

make:
	nmake -f ./bin/Makefile.mk
movefiles:
	mv -f ./*.exe ./bin/
	mv -f ./*.obj ./bin/
	mv ./bin/ConsolaControl.exe ./bin/conctrl.exe
	mv ./bin/ProcesoControl.exe ./bin/procesoctrl.exe
clean:
	del *.c~ *.obj *.exe Makefile.mk~