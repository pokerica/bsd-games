################ Source files ##########################################

fish/NAME	:= fish
fish/EXE	:= $Ofish/${fish/NAME}
fish/SRCS	:= $(wildcard fish/*.c)
fish/OBJS	:= $(addprefix $O,$(fish/SRCS:.c=.o))
fish/DEPS	:= $(fish/OBJS:.o=.d)
fish/LIBS	:= ${COMLIB} ${CURSES_LIBS}

################ Compilation ###########################################

.PHONY:	fish/all fish/clean fish/run fish/install fish/uninstall

all:		fish/all
fish/all:	${fish/EXE}
fish/run:	${fish/EXE}
	@${fish/EXE}

${fish/EXE}:	${fish/OBJS} ${COMLIB}
	@echo "Linking $@ ..."
	@${CC} ${LDFLAGS} -o $@ ${fish/OBJS} ${fish/LIBS}

################ Installation ##########################################

ifdef BINDIR
fish/EXEI	:= ${BINDIR}/${fish/NAME}
fish/MANI	:= ${MANDIR}/man6/${fish/NAME}.6.gz

install:		fish/install
fish/install:	${fish/EXEI} ${fish/MANI}
${fish/EXEI}:	${fish/EXE}
	@echo "Installing $@ ..."
	@${INSTALLEXE} $< $@
${fish/MANI}:	fish/${fish/NAME}.6
	@echo "Installing $@ ..."
	@gzip -9 -c $< > $@ && chmod 644 $@

uninstall:		fish/uninstall
fish/uninstall:
	@if [ -f ${fish/EXEI} ]; then\
	    echo "Removing ${fish/EXEI} ...";\
	    rm -f ${fish/EXEI} ${fish/MANI};\
	fi
endif

################ Maintenance ###########################################

clean:	fish/clean
fish/clean:
	@if [ -d $O/fish ]; then\
	    rm -f ${fish/EXE} ${fish/OBJS} ${fish/DEPS} $Ofish/.d;\
	    rmdir $O/fish;\
	fi

$Ofish/.d:	$O.d
	@[ -d $Ofish ] || mkdir $Ofish && touch $Ofish/.d

${fish/OBJS}: ${CONFS} fish/Module.mk $Ofish/.d

-include ${fish/DEPS}
