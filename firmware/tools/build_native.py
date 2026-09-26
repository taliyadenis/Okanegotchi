"""Build the actual portable firmware with a desktop C++17 compiler."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
ROOT=Path(__file__).resolve().parents[1]
if __name__=='__main__':
    ap=argparse.ArgumentParser();ap.add_argument('--compiler',default=shutil.which('g++') or 'C:/msys64/ucrt64/bin/g++.exe');ap.add_argument('--arduinojson',type=Path,required=True);a=ap.parse_args()
    (ROOT/'build').mkdir(exist_ok=True)
    sources=['protocol.cpp','engine.cpp','demo.cpp','assets.cpp','generated/assets_data.cpp','core/button_input.cpp','nfc/ndef_parser.cpp']
    cmd=[a.compiler,'-std=c++17','-O1','-Wall','-Wextra','-Wno-deprecated-declarations','-I'+str(ROOT/'src'),'-I'+str(a.arduinojson/'src'),*[str(ROOT/'src'/s) for s in sources],str(ROOT/'tests/host_driver.cpp'),'-o',str(ROOT/'build'/('host_driver.exe' if os.name=='nt' else 'host_driver'))]
    subprocess.run(cmd,check=True)
    portable=['protocol.cpp','engine.cpp','demo.cpp','core/button_input.cpp','nfc/ndef_parser.cpp','platform/pn532.cpp']
    cmd=[a.compiler,'-std=c++17','-O1','-Wall','-Wextra','-Wno-deprecated-declarations','-DARDUINO=1','-DARDUINOJSON_ENABLE_ARDUINO_STRING=0','-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0','-DARDUINOJSON_ENABLE_PROGMEM=0','-I'+str(ROOT/'tests/fakes'),'-I'+str(ROOT/'src'),'-I'+str(a.arduinojson/'src'),*[str(ROOT/'src'/s) for s in portable],str(ROOT/'tests/pn532_test.cpp'),'-o',str(ROOT/'build'/('pn532_test.exe' if os.name=='nt' else 'pn532_test'))]
    subprocess.run(cmd,check=True)
    env=os.environ.copy();env['PATH']=str(Path(a.compiler).resolve().parent)+os.pathsep+env['PATH']
    subprocess.run([str(ROOT/'build'/('pn532_test.exe' if os.name=='nt' else 'pn532_test'))],check=True,env=env)
