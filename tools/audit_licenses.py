"""Record the actual MSBuild runtime dependency graph; fail on unknown origins.

This checks provenance/declared terms, not legal certification. The official component-specific Cog notice and per-file exceptions are recorded.
No rights-holder contact or blanket legal certification is implied.
"""
from pathlib import Path
import hashlib,json,re,sys,xml.etree.ElementTree as ET
src=Path(__file__).resolve().parent.parent
build=Path(sys.argv[1]).resolve()
projects=set();files={}
ns={'m':'http://schemas.microsoft.com/developer/msbuild/2003'}
def visit(project):
    project=project.resolve()
    if project in projects:return
    projects.add(project);root=ET.parse(project).getroot()
    for e in root.findall('.//m:ProjectReference',ns):
        if 'Include' in e.attrib:visit(project.parent/e.attrib['Include'])
    for e in root.findall('.//m:ClCompile',ns):
        if 'Include' not in e.attrib:continue
        f=(project.parent/e.attrib['Include']).resolve()
        if f.is_file():files.setdefault(f,set()).add(project.stem)
visit(build/'NintendoGameMusicViewer.vcxproj')
inventory=[];unknown=[];conflicts=[]
for f,targets in sorted(files.items()):
    data=f.read_bytes();text=data.decode('utf-8',errors='replace')
    name=f.as_posix();rel=None;evidence=None;license=None
    try:rel=f.relative_to(src).as_posix()
    except ValueError:pass
    if rel:
        if rel.startswith('src/'):
            license='LGPL-2.1-or-later' if rel.endswith('/NsfInspect.cpp') else 'GPL-2.0-or-later'
            evidence='SPDX/header or project LICENSE + THIRD_PARTY_NOTICES.md'
        elif rel.startswith('third_party/licensed-chips/'):
            license='LGPL-2.1-or-later' if f.name.startswith('ym3438') else 'GPL-2.0-or-later' if f.name=='fmopl.c' else 'BSD-3-Clause'
            evidence='Explicit first-line core license + third_party/licensed-chips/UPSTREAM.json'
        elif rel.startswith('third_party/vgsound-es5506/'):
            license='Zlib';evidence='Per-file License:Zlib and third_party/vgsound-es5506/LICENSE'
        elif rel.startswith('third_party/viogsf/'):
            license='LGPL-2.1-or-later' if '/apu/' in rel else 'GPL-2.0-or-later';evidence='third_party/viogsf/vbam/License.txt and per-file headers'
        elif rel.startswith('third_party/vio2sf/'):
            license='GPL-2.0-or-later';evidence='third_party/vio2sf/src/vio2sf/desmume/COPYING and headers'
        elif rel.startswith('third_party/lazyusf/'):
            license='CC0-1.0' if '/rsp_lle/' in rel else 'BSD-2-Clause' if f.name=='n64_cic_nus_6105.c' else 'GPL-2.0-or-later'
            evidence='Pinned official Cog lazyusf2 component declaration: LICENSE-EVIDENCE.json / Cog-Acknowledgements.json; per-file GPL/CC0/BSD notices'
            if re.search(r'non-commercial purposes|PERSONAL USE only',text,re.I):
                conflicts.append(rel)
        elif rel.startswith('third_party/vgmstream/'):
            license='ISC-style/per-file notices';evidence='third_party/vgmstream/COPYING and per-file notices'
        elif rel.startswith('third_party/zlib/'):
            license='Zlib';evidence='third_party/zlib/LICENSE'
    elif '/libgme-src/' in name:
        rel='@gme/'+name.split('/libgme-src/',1)[1]
        license='MIT' if 'emu2413' in f.name.lower() else 'LGPL-2.1-or-later';evidence='Bundled0.6.5 source archive, LGPL/MIT notices and complete local patches'
    elif '/sdl-src/' in name:
        rel='@sdl/'+name.split('/sdl-src/',1)[1];license='Zlib';evidence='Bundled SDL2 archive LICENSE.txt'
    if not license:unknown.append(name)
    inventory.append({'path':rel or name,'SHA256':hashlib.sha256(data).hexdigest(),'targets':sorted(targets),'declared_terms':license,'evidence':evidence})
assert not unknown,unknown
assert not any('/third_party/libvgm/' in str(p).replace('\\','/') or '/es5506-legacy/' in str(p).replace('\\','/') for p in files)
assert not(src/'third_party/libvgm').exists() and not(src/'third_party/es5506-legacy').exists()
assert not conflicts,conflicts
assert not list((src/'third_party').glob('audiodecoder.usf-*.zip'))
result={'date':'2026-11-01','version':'1.5','scope':'Actual runtime transitive MSBuild compile inputs; file hashes and module/per-file notice mapping. Headers/source archive notices retained separately. Not a legal opinion.','runtime_projects':len(projects),'compiled_translation_units':len(files),'unclassified_translation_units':unknown,'removed_VGM_framework_and_old_ES':True,'remaining_conflicting_N64_TUs':conflicts,'identified_license_blockers_remaining':len(conflicts),'public_release_package':True,'external_upload':False,'legal_certification':False,'N64_user_instruction':'Keep N64; research only, no public posting/contact. Licensed replacement retained N64.','files':inventory}
output=Path(sys.argv[2]) if len(sys.argv)>2 else src/'LICENSE-INVENTORY.json'
output.write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k!='files'},indent=2))
