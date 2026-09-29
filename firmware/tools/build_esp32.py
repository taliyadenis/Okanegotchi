"""Compile without uploading. --workspace-tools uses the local Arduino installation.

Windows --short-path maps the workspace to an unused drive during the build to
avoid Xtensa GCC's long include-path failures. The mapping is removed in finally.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
ROOT=Path(__file__).resolve().parents[1]
FQBN='esp32:esp32:esp32s3:FlashSize=4M,PSRAM=enabled,PartitionScheme=huge_app,CDCOnBoot=cdc,USBMode=hwcdc'
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--demo',action='store_true');ap.add_argument('--workspace-tools',type=Path);ap.add_argument('--short-path',action='store_true');ap.add_argument('--verbose',action='store_true');a=ap.parse_args()
    mapping=None;root=ROOT;tools=a.workspace_tools.resolve() if a.workspace_tools else None
    try:
        if a.short_path:
            if os.name!='nt' or not tools:raise SystemExit('--short-path requires Windows and --workspace-tools')
            workspace=tools.parent
            if not root.is_relative_to(workspace):raise SystemExit('firmware must be under the tools workspace')
            for letter in 'ZYXWVUTSRQP':
                drive=letter+':'
                if not Path(drive+'/').exists():
                    result=subprocess.run(['subst',drive,str(workspace)],capture_output=True)
                    if result.returncode==0:mapping=drive;break
            if not mapping:raise SystemExit('No available short drive mapping')
            root=Path(mapping+'/')/ROOT.relative_to(workspace);tools=Path(mapping+'/')/tools.relative_to(workspace)
        mode='local-demo' if a.demo else 'cloud';build=root/'build'/('esp32-'+mode);build.mkdir(parents=True,exist_ok=True)
        cmd=[str(tools/'arduino/arduino-cli.exe') if tools else (shutil.which('arduino-cli') or 'arduino-cli')]
        if tools:
            config=root/'build/arduino-build.yaml';config.write_text('directories:\n'+''.join('  '+key+': '+(tools/value).as_posix()+'\n' for key,value in [('data','arduino-data'),('downloads','arduino-downloads'),('user','arduino-user')])+'board_manager:\n  additional_urls:\n    - https://espressif.github.io/arduino-esp32/package_esp32_index.json\n')
            cmd+=['--config-file',str(config)]
        cmd+=['compile','--fqbn',FQBN,'--library',str(root),'--build-path',str(build),'--build-property',f'compiler.cpp.extra_flags=-DOKANEGACHI_LOCAL_DEMO={int(a.demo)}',str(root/'examples/Okanegachi')]
        if a.verbose:cmd.append('--verbose')
        result=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
        (ROOT/'build'/('compile-'+mode+'.log')).write_text(result.stdout,encoding='utf-8')
        print(result.stdout[-14000:] if result.returncode else result.stdout)
        return result.returncode
    finally:
        if mapping:subprocess.run(['subst',mapping,'/D'],check=True)
if __name__=='__main__':raise SystemExit(main())
