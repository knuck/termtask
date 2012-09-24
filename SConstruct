
env = Environment(CCFLAGS='-std=c++0x')

Export('env')

t = env.Program('termtask',['main.cpp',
							'x11.cpp',
							'formatting.cpp',
							'io.cpp',
							'helpers.cpp',
							'settings.cpp',
							'fail.cpp'],LIBS=['X11','stdc++'])
Default(t)