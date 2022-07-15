# Minimal Makefile for pstree (c) Fred Hucht 2022

CFLAGS = -O

pstree: pstree.c

clean:
	$(RM) pstree
