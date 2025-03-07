# Copyright (c) 2017 Linaro Limited.
#
# SPDX-License-Identifier: Apache-2.0

'''Runner for pyOCD .'''

import os

from runners.core import WellsL4BinaryRunner, RunnerCaps, \
    BuildConfiguration

DEFAULT_PYOCD_GDB_PORT = 3333
DEFAULT_PYOCD_TELNET_PORT = 4444


class PyOcdBinaryRunner(WellsL4BinaryRunner):
    '''Runner front-end for pyOCD.'''

    def __init__(self, cfg, target,
                 pyocd='pyocd',
                 flash_addr=0x0, flash_opts=None,
                 gdb_port=DEFAULT_PYOCD_GDB_PORT,
                 telnet_port=DEFAULT_PYOCD_TELNET_PORT, tui=False,
                 board_id=None, daparg=None, frequency=None):
        super(PyOcdBinaryRunner, self).__init__(cfg)

        self.target_args = ['-t', target]
        self.pyocd = pyocd
        self.flash_addr_args = ['-a', hex(flash_addr)] if flash_addr else []
        self.gdb_cmd = [cfg.gdb] if cfg.gdb is not None else None
        self.gdb_port = gdb_port
        self.telnet_port = telnet_port
        self.tui_args = ['-tui'] if tui else []
        self.hex_name = cfg.hex_file
        self.bin_name = cfg.bin_file
        self.elf_name = cfg.elf_file

        board_args = []
        if board_id is not None:
            board_args = ['-u', board_id]
        self.board_args = board_args

        daparg_args = []
        if daparg is not None:
            daparg_args = ['-da', daparg]
        self.daparg_args = daparg_args

        frequency_args = []
        if frequency is not None:
            frequency_args = ['-f', frequency]
        self.frequency_args = frequency_args

        self.flash_extra = flash_opts if flash_opts else []

    @classmethod
    def name(cls):
        return 'pyocd'

    @classmethod
    def capabilities(cls):
        return RunnerCaps(commands={'flash', 'debug', 'debugserver', 'attach'},
                          flash_addr=True)

    @classmethod
    def do_add_parser(cls, parser):
        parser.add_argument('--target', required=True,
                            help='target override')

        parser.add_argument('--daparg',
                            help='Additional -da arguments to pyocd tool')
        parser.add_argument('--pyocd', default='pyocd',
                            help='path to pyocd tool, default is pyocd')
        parser.add_argument('--flash-opt', default=[], action='append',
                            help='''Additional options for pyocd flash,
                            e.g. --flash-opt="-e=chip" to chip erase''')
        parser.add_argument('--frequency',
                            help='SWD clock frequency in Hz')
        parser.add_argument('--gdb-port', default=DEFAULT_PYOCD_GDB_PORT,
                            help='pyocd gdb port, defaults to {}'.format(
                                DEFAULT_PYOCD_GDB_PORT))
        parser.add_argument('--telnet-port', default=DEFAULT_PYOCD_TELNET_PORT,
                            help='pyocd telnet port, defaults to {}'.format(
                                DEFAULT_PYOCD_TELNET_PORT))
        parser.add_argument('--tui', default=False, action='store_true',
                            help='if given, GDB uses -tui')
        parser.add_argument('--board-id',
                            help='ID of board to flash, default is to prompt')

    @classmethod
    def create(cls, cfg, args):
        build_conf = BuildConfiguration(cfg.build_dir)
        flash_addr = cls.get_flash_address(args, build_conf)

        ret = PyOcdBinaryRunner(
            cfg, args.target,
            pyocd=args.pyocd,
            flash_addr=flash_addr, flash_opts=args.flash_opt,
            gdb_port=args.gdb_port, telnet_port=args.telnet_port, tui=args.tui,
            board_id=args.board_id, daparg=args.daparg,
            frequency=args.frequency)

        daparg = os.environ.get('PYOCD_DAPARG')
        if not ret.daparg_args and daparg:
            ret.logger.warning('PYOCD_DAPARG is deprecated; use --daparg')
            ret.logger.debug('--daparg={} via PYOCD_DAPARG'.format(daparg))
            ret.daparg_args = ['-da', daparg]

        return ret

    def port_args(self):
        return ['-p', str(self.gdb_port), '-T', str(self.telnet_port)]

    def do_run(self, command, **kwargs):
        self.require(self.pyocd)
        if command == 'flash':
            self.flash(**kwargs)
        else:
            self.debug_debugserver(command, **kwargs)

    def flash(self, **kwargs):
        if os.path.isfile(self.hex_name):
            fname = self.hex_name
        elif os.path.isfile(self.bin_name):
            self.logger.warning(
                'hex file ({}) does not exist; falling back on .bin ({}). '.
                format(self.hex_name, self.bin_name) +
                'Consider enabling CONFIG_BUILD_OUTPUT_HEX.')
            fname = self.bin_name
        else:
            raise ValueError(
                'Cannot flash; no hex ({}) or bin ({}) files found. '.format(
                    self.hex_name, self.bin_name))

        cmd = ([self.pyocd] +
               ['flash'] +
               ['-e', 'sector'] +
               self.flash_addr_args +
               self.daparg_args +
               self.target_args +
               self.board_args +
               self.frequency_args +
               self.flash_extra +
               [fname])

        self.logger.info('Flashing file: {}'.format(fname))
        self.check_call(cmd)

    def log_gdbserver_message(self):
        self.logger.info('pyOCD GDB server running on port {}'.
                         format(self.gdb_port))

    def debug_debugserver(self, command, **kwargs):
        server_cmd = ([self.pyocd] +
                      ['gdbserver'] +
                      self.daparg_args +
                      self.port_args() +
                      self.target_args +
                      self.board_args +
                      self.frequency_args)

        if command == 'debugserver':
            self.log_gdbserver_message()
            self.check_call(server_cmd)
        else:
            if self.gdb_cmd is None:
                raise ValueError('Cannot debug; gdb is missing')
            if self.elf_name is None:
                raise ValueError('Cannot debug; elf is missing')
            client_cmd = (self.gdb_cmd +
                          self.tui_args +
                          [self.elf_name] +
                          ['-ex', 'target remote :{}'.format(self.gdb_port)])
            if command == 'debug':
                client_cmd += ['-ex', 'monitor halt',
                               '-ex', 'monitor reset',
                               '-ex', 'load']

            self.require(client_cmd[0])
            self.log_gdbserver_message()
            self.run_server_and_client(server_cmd, client_cmd)
