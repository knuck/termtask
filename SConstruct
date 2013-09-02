AddOption(
    '--dbg',
    action='store_true',
    help='debug build',
    default=False)

AddOption(
    '--ast',
    action='store_true',
    help='output ast',
    default=False)

env = Environment(CCFLAGS='-std=c++11 -Wfatal-errors')

if GetOption('dbg'):
    env.Append(CCFLAGS = ' -g')
if GetOption('ast'):
	env.Append(CCFLAGS= '-fdump-tree-vcg')
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
							'task.cpp',
							'ncurses.cpp',
							'ncurses_widget.cpp',
							'ncurses_listbox.cpp',
							'ncurses_table.cpp',],LIBS=['X11','stdc++','menu','ncurses'])
Default(t)