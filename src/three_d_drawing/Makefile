#*******************************************************************************
# FILE : Makefile
#
# LAST MODIFIED : 17 August 1998
#
# DESCRIPTION :
# Makefile for the 3-D drawing widget test program.
#===============================================================================

CC = cc -n32

CFLAGS = -ansi -pedantic
#CFLAGS = -ansi -pedantic -fullwarn -woff 841
#CFLAGS =

#OPTION_FLAGS = -DGL_API -DSGI
OPTION_FLAGS = -DOPENGL_API
#OPTION_FLAGS = -DGL_API -DIBM
#OPTION_FLAGS = -DPEXLIB_API -DIBM
#OPTION_FLAGS = /define=(OPENGL_API,INSTALL_COLOURMAP)
#OPTION_FLAGS = /define=(PEXLIB_API,VAX)

#LIB = -lMrm -lXm -lXt -lX11 -lgl -lPW
#LIB = -lMrm -lXm -lXt -lX11 -lPEX5 -lPW -lm
#LIB = -lgl_s -lMrm -lXm_s -lXt_s -lX11_s -lPW -lm
#LIB = -lMrm -lXm_s -lXt_s -lX11_s -lPW -lGL -lGLw -lGLU
LIB = -lMrm -lXm -lXt -lX11 -lPW -lGL -lGLw -lGLU
#LIB = alpha.opt/opt

OBJS = test_3d.o ThreeDDraw.o

all : test_3d

clean :
	rm *.o
	rm test_3d

tar :
	tar -cvf/dev/fd0 Makefile *.mms *.opt *.c *.h

transfer :
	tar -cvf three_d_drawing.tar Makefile *.mms *.opt *.c *.h

compare :
	mv three_d_drawing.tar tmp
	(cd tmp; tar -xvf three_d_drawing.tar >& tar.list; \
sed "s/./diff/;s/,.*/ >> out.diff/;h;G;G;s/\n//2;s%>> out\.diffdiff %tmp/%;s/diff/echo FILE:/" \
<tar.list >compare.script; rm tar.list)
	mv tmp/three_d_drawing.tar .
	sh tmp/compare.script

test_3d : $(OBJS)
	$(CC) -o test_3d $(OBJS) $(LIB)

test_3d.o : test_3d.c ThreeDDraw.h
	$(CC) -c $(CFLAGS) $(OPTION_FLAGS) -o test_3d.o test_3d.c

ThreeDDraw.o : ThreeDDraw.c ThreeDDraP.h ThreeDDraw.h
	$(CC) -c $(CFLAGS) $(OPTION_FLAGS) -o ThreeDDraw.o ThreeDDraw.c

ThreeDDraw.h : X3d.h
	touch -c ThreeDDraw.h
