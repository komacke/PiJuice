PACKAGE = $(shell basename $(spec)/*.spec)
srpm:
	dnf install -y i2c-tools python3-i2c-tools
	COPR=/builddir/build ./first-prep-for-rpmbuild.sh 
	rpmbuild -bs $(PACKAGE)
	cp -r /builddir/build/SRPMS/* $(outdir)
