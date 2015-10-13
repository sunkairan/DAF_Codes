import os
import sys

EnsureSConsVersion( 1, 1, 0 )

def add_option(name, help, nargs, 
               dest = None, default = None, type = "string", choices = None):

    if dest is None:
        dest = name

        AddOption("--" + name,
                  dest=dest,
                  type=type,
                  nargs=nargs,
                  action="store",
                  choices=choices,
                  default=default,
                  help=help)

def get_option( name ):
    return GetOption( name )


def has_option( name ):
    x = get_option( name )
    if x is None:
        return False

    if x == False:
        return False

    if x == "":
        return False

    return True


# --- command line options supported ---
add_option("d", "debug build with no optimization", 0)
add_option("dd", "debug build with no optimization, additional debug logging, etc...", 0)
add_option("nosse", "disable SSE instruction", 0)
add_option("cc", "choose a c/c++ compiler: gcc/clang", 1)
add_option("64", "compile it using 64 bit instruction", 0)
add_option("32", "compile it using 32 bit instruction", 0)
add_option("r", "debug with rank distribution", 0)
add_option("vand", "use Vandermond Matrix as Generator Matrix", 0)

# --- get command line option for configuration ---
cc = get_option("cc")
nosse = has_option("nosse")
debug = has_option("d") or has_option("dd")
debug_loging = has_option("dd")
force64 = has_option("64")
force32 = has_option("32")
rank = has_option("r")
vand = has_option("vand")

# --- environment setup ---
env = Environment()

if rank:
    env.Append(CPPDEFINES=["RANK_DEBUG"])

if vand:
    env.Append(CPPDEFINES=["USE_VANDERMOND"])

if debug:
    env.Append(CCFLAGS=["-O0", "-fstack-protector"])
    env.Append(CCFLAGS=["-g"])
else:
    env.Append(CCFLAGS=["-O3"])

if debug_loging:
    env.Append(CPPDEFINES=["LOG_DEBUG"])

env.Append(CCFLAGS=["-msse2"])
env.Append(CCFLAGS=["-mstackrealign"])

# use sse for boost
if not nosse:
    env.Append(CPPDEFINES=["SSE_DEBUG"])
    env.Append(CPPDEFINES=["SSE_BOOST"])
    

env.Append(CPPDEFINES=["PRINT_DEBUG", "ERROR_DEBUG"])
env.Append(CPPPATH=["#src/"])


# determin processor type
platform = os.sys.platform
if "uname" in dir(os):
    processor = os.uname()[4] # x86-64
else:
    processor = "i386"

env['PROCESSOR_ARCHITECTURE'] = processor

if force64:
    env.Append(CCFLAGS=["-m64"])
    env.Append(LINKFLAGS=["-m64"])
if force32:
    env.Append(CCFLAGS=["-m32"])
    env.Append(LINKFLAGS=["-m32"])



# determin os type
nix = False
linux = False
darwin = False

# determin os platform type
if "darwin" == os.sys.platform:
    darwin = True
    platform = 'osx'
    env['CC'] = 'clang++'
    env['CXX'] = 'clang++'

    if cc is not None:
        if cc == 'gcc':
            env['CC'] = 'g++'
            env['CXX'] = 'g++'

    nix = True

elif os.sys.platform.startswith("linux"):
    linux = True
    platform = "linux"
    env['CC'] = 'g++'
    env['CXX'] = 'g++'

    # deal with PlanetLab
    if cc is not None:
        env['CC'] = cc + '/bin' + '/g++'
        env['CXX'] = cc + '/bin' + '/g++'

        include_path = cc + '/include/c++/4.7.2/'
        env.Append(CPPPATH=[include_path])
        
        include_path = cc + '/include/c++/4.7.2/i686-pc-linux-gnu/'
        env.Append(CPPPATH=[include_path])
        
        env.Append(CCFLAGS=["-Wl,-rpath=/opt/gcc-4.7.2/lib"])
        env.Append(LINKFLAGS=["-Wl,-rpath=/opt/gcc-4.7.2/lib"])
        lib_path = cc + '/lib/'
        env.Append(LINKPATH=[lib_path])

    nix = True

elif "win32" == os.sys.platform:
    windows = True



Export("env")    

env.SConscript('src/SConscript', variant_dir='build', duplicate=False)
env.SConscript('test/SConscript', duplicate=False)


