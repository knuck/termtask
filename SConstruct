AddOption(
    '--dbg',
    action='store_true',
    help='debug build',
    default=False)
env = Environment(CCFLAGS='-std=c++0x')

if GetOption('dbg'):
    env.Append(CCFLAGS = ' -g')

Export('env')

t = env.Program('termtask',['main.cpp',
							'x11.cpp',
							'formatting.cpp',
							'io.cpp',
							'helpers.cpp',
							'settings.cpp',
							'fail.cpp',
							'workspace.cpp',
							'init.cpp',
							'task.cpp'],LIBS=['X11','stdc++'])
Default(t)