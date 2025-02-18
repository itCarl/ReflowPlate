Import("env")
import datetime

build_time = datetime.datetime.now().strftime('%d.%m.%Y %H:%M:%S')
version_code = int(datetime.datetime.now().strftime('%y%m%d0'))

env.Append(CPPDEFINES=[
    ('BUILD_TIME', f'\\"{build_time}\\"'),
    ('VERSION_CODE', f'\\"{version_code}\\"')
])
