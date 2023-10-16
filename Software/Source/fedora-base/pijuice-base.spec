Name:           pijuice-base
Version:        
Release:        1%{?dist}
Summary:        

License:        
URL:            
Source0:        

BuildRequires:  
Requires:       python3-i2c-tools, python3-tkinter, python3-urwid

%description
first pass at pijuice fedora support


%prep
%autosetup


%build
%configure
%make_build


%install
%make_install


%files
%license add-license-file-here
%doc add-docs-here



%changelog
* Mon Oct 16 2023 Dave Koberstein <davek@komacke.com>
- 
