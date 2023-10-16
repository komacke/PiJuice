Name:           pijuice-gui
Version:        
Release:        1%{?dist}
Summary:        

License:        
URL:            
Source0:        

BuildRequires:  
Requires:       pijuice-base

%description
first pass at pijuice-gui fedora support


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
