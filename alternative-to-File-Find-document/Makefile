DOC = alternatives-to-File-Find

DEST_DIR = $(DOC)
DEST_INDEX = $(DEST_DIR)/index.html

all: html-xsl

DOCMAKE_SGML_PATH = ./sgml/shlomif-docbook/

XMLTO_PARAMS = --stringparam "docmake.output.path_to_root=$(shell perl -e '$$_=shift;$$c=tr[/][];print "../"x($$c+2)' $(call get,DOCBOOK_DIRS_MAP,$(patsubst $(DOCBOOK_INDIVIDUAL_XHTML_DIR)/%,%,$@)))"

validate: $(DOC).xml
	xmllint --relaxng http://www.docbook.org/xml/5.0/rng/docbook.rng $<

html-xsl: $(DEST_INDEX) $(DOC)/style.css put-images

$(DEST_INDEX): $(DOC).xml
	xsltproc --path /usr/share/sgml/docbook/xsl-stylesheets-db5-1.73.2/xhtml -o $@ docbook.xsl $<

$(DOC)/style.css: style.css
	cp -f $< $@

firefox: html-xsl
	mozilla-firefox $(DEST_INDEX)

upload: html-xsl
	rsync -a -v --progress $(DEST_DIR) $${HOMEPAGE_SSH_PATH}/hebrew-html-tutorial-brk800/

edit:
	gvim $(DOC).xml style.css Summary.txt

IMAGES_PROTO = 

IMAGES = $(patsubst images/%,%,$(IMAGES_PROTO))

DEST_IMAGES_DIR = $(DEST_DIR)/images

DEST_IMAGES = $(addprefix $(DEST_IMAGES_DIR)/,$(IMAGES))

put-images: $(DEST_IMAGES_DIR) $(DEST_IMAGES)

$(DEST_IMAGES_DIR):
	mkdir -p $@

$(DEST_IMAGES) : $(DEST_IMAGES_DIR)/% : images/%
	cp -f $< $@
