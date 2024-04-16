project('data_management_equipment_utilization_capture', 'c', version: '0.1.0')


cmd = run_command('bash', '-c', 'echo $HOME', check: false)
home_dir = cmd.stdout().strip()

executable(
  'safer',
  [
    'src/gui.c',
    'src/gui.h',
    'src/gui_functions.c',
    'src/gui_functions.h',
    'src/main.c',
    'src/power-monitor.c',
    'src/power-monitor.h',
    'src/utils.h',
    'src/utils.c',
    'src/worker.h',
    'src/worker.c'
  ],
  dependencies : [
    dependency('glib-2.0', required: true),
    dependency('gtk+-3.0', required: true),
    dependency('hiredis', required: true),
    dependency('libpcre2-8', required: true),
    dependency('python3', modules: ['redis'], required: true),
    dependency('libsystemd', required: true)
  ],
  link_args: '-lm',
  install: true,
  install_dir: home_dir / 'bin'
)
install_data('python/bat.py', install_dir: home_dir / 'bin')
install_data('python/bat.service', install_dir: home_dir / '.config/systemd/user')
install_data('python/hat.py', install_dir: home_dir / 'bin')
install_data('theme.css', install_dir: home_dir)

run_target(
  'cppcheck',
  command : [
    'cppcheck',
    '--enable=all',
    '--suppress=missingIncludeSystem',
    '--project=' + join_paths(meson.build_root(),
    'compile_commands.json')
  ]
)

