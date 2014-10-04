README: .readme
	groff -Tascii $< >$@

clean:
	rm -f README
