.phony: ProcesoSuicida ConsolaControl ProcesoControl

ProcesoSuicida: 
	cl -W 3 ./src/Windows/ProcesoSuicida.c
ConsolaControl:
	cl -W 3 ./src/Windows/ConsolaControl.c
ProcesoControl:
	cl -W 3 ./src/Windows/ProcesoControl.c
clean:
	del *.c~ *.obj *.exe Makefile.mk~