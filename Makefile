OBJS	=	readmap.o 
CFLAGS	=	-g

mkheader:	${OBJS} mkheader.o
all:		fiswmap.h
fiswmap.h:	mkheader
		./mkheader fiswmap < data/1387901-fi-sw.txt > fiswmap.h
