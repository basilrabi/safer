project('data_management_equipment_utilization_capture', 'c', version: '0.1.0')

gtk_dep = dependency('gtk+-3.0', required: true)
hiredis_dep = dependency('hiredis', required: true)
pcre_dep = dependency('libpcre2-8', required: true)
python_dep = dependency('python3', modules: ['redis'], required: true)
systemd_dep = dependency('libsystemd', required: true)

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
    'src/utils.h',
    'src/utils.c',
    'src/worker.h',
    'src/worker.c'
  ],
  dependencies : [gtk_dep, hiredis_dep, pcre_dep, python_dep, systemd_dep],
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

