PROG = twitterm
DIR = src

bin:
	cd ${DIR}; make bin
	mv ${DIR}/${PROG} .

srcclean:
	cd ${DIR}; make clean;
	rm -f ${PROG}

doc:
	cd user_manual; latex userman.tex
	doxygen && doxygen
	# remove subsubsections from the latex files since they're completely
	# unnecessary. The script is not inline
	./removesssections.sh
	cd latex; make

indent:
	./indent.sh

docclean:
	rm -rf latex/
	rm -f doxygen.log

clean: srcclean docclean

all: bin doc
