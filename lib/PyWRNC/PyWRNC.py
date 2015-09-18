"""@package docstring
@author: Federico Vaga <federico.vaga@cern.ch>
@copyright: Copyright (c) 2015 CERN
@license: GNU Public License version 3
"""

from ctypes import *
import errno
import os


class WrncMessageInternal(Structure):
    _fields_ = [
        ("datalen", c_uint32),
        ("data", c_uint32 * 128),
        ("max_size", c_uint32),
        ("offset", c_uint32),
        ("direction", c_int),
        ("error", c_int),
    ]

class WrncHeader(Structure):
    _fields_ = [
        ("rt_app_id", c_uint16),
        ("msg_id", c_uint8),
        ("slot_io", c_uint8),
        ("seq",  c_uint32),
        ("len",  c_uint8),
        ("flags",  c_uint8),
        ("__unused",  c_uint8),
        ("trans",  c_uint8),
        ("time",  c_uint32),
    ]

class WrncMessage(Structure):
    _fields_ = [
        ("header", WrncHeader),
        ("payload", (c_uint32 * (128 - 4))),
    ]


class Wrnc(object):
    """
    This is an abstract class to handle the WRNC library
    """
    def __init__(self, lun=None, device_id=None):
        self.libwrnc = CDLL("libwrnc.so", use_errno=True)
        self.lun = lun
        self.device_id = device_id

        set_errno(0)
        self.libwrnc.wrnc_init()

        self.device = None
        if device_id is not None:
            self.device = self.libwrnc.wrnc_open_by_fmc(self.device_id)
        if lun is not None:
            self.device = self.libwrnc.wrnc_open_by_lun(self.lun)

        if self.device is None:
            raise OSError(get_errno(), os.strerror(get_errno()), "")

    def __del__(self):
        self.libwrnc.close(self.device)
        self.libwrnc.wrnc_exit()

    def errcheck(self, ret, func, args):
        """
        Generic error checker
        """
        if func == self.libwrnc.wrnc_hmq_receive:
            if ret is None and get_errno() != 0:
                raise OSError(get_errno(), os.strerror(get_errno()), "")
            else:
                return None

        if func == self.libwrnc.wrnc_debug_open or \
           func == self.libwrnc.wrnc_hmq_open:
            if ret is None:
                raise OSError(get_errno(), os.strerror(get_errno()), "")
            else:
                return ret

        if ret < 0:
            raise OSError(get_errno(), os.strerror(get_errno()), "")
        else:
            return ret


class WrncCpu(Wrnc):
    """
    Python wrapper for CPU management
    """

    def __init__(self, lun=None, device_id=None,
                 cpu_index=0):
        super(WrncCpu, self).__init__(lun, device_id)
        self.cpu_index = cpu_index

        # Arguments
        self.libwrnc.wrnc_cpu_enable.argtypes = [c_void_p, c_uint]
        self.libwrnc.wrnc_cpu_disable.argtypes = [c_void_p, c_uint]
        self.libwrnc.wrnc_cpu_start.argtypes = [c_void_p, c_uint]
        self.libwrnc.wrnc_cpu_stop.argtypes = [c_void_p, c_uint]
        self.libwrnc.wrnc_cpu_load_application_file.argtypes = [c_void_p,
                                                                c_uint,
                                                                c_char_p]
        self.libwrnc.wrnc_cpu_dump_application_file.argtypes = [c_void_p,
                                                                c_uint,
                                                                c_char_p]
        # Return
        self.libwrnc.wrnc_cpu_enable.restype = c_int
        self.libwrnc.wrnc_cpu_disable.restype = c_int
        self.libwrnc.wrnc_cpu_is_enable.restype = c_int
        self.libwrnc.wrnc_cpu_start.restype = c_int
        self.libwrnc.wrnc_cpu_stop.restype = c_int
        self.libwrnc.wrnc_cpu_is_running.restype = c_int
        self.libwrnc.wrnc_cpu_load_application_file.restype = c_int
        self.libwrnc.wrnc_cpu_dump_application_file.restype = c_int
        # Error
        self.libwrnc.wrnc_cpu_enable.errcheck = self.errcheck
        self.libwrnc.wrnc_cpu_disable.errcheck = self.errcheck
        self.libwrnc.wrnc_cpu_is_enable.errcheck = self.errcheck
        self.libwrnc.wrnc_cpu_start.errcheck = self.errcheck
        self.libwrnc.wrnc_cpu_stop.errcheck = self.errcheck
        self.libwrnc.wrnc_cpu_is_running.errcheck = self.errcheck
        self.libwrnc.wrnc_cpu_load_application_file.errcheck = self.errcheck
        self.libwrnc.wrnc_cpu_dump_application_file.errcheck = self.errcheck

    def enable(self):
        """
        It enables a CPU; in other words, it clear the reset line of a CPU.
        @exception OSError from C library errors
        """
        self.libwrnc.wrnc_cpu_enable(self.device, self.cpu_index)

    def disable(self):
        """
        It disables a CPU; in other words, it sets the reset line of a CPU.
        @exception OSError from C library errors
        """
        self.libwrnc.wrnc_cpu_disable(self.device, self.cpu_index)

    def is_enable(self):
        """
        It checks if the CPU is enabled (or not)
        @return True when the CPU is enable, False otherwise
        @exception OSError from C library errors
        """
        enable = c_int(0)
        self.libwrnc.wrnc_cpu_is_enable(self.device, self.cpu_index,
                                        byref(enable))
        return True if enable.value else False

    def start(self):
        """
        It starts to execute code
        @exception OSError from C library errors
        """
        self.libwrnc.wrnc_cpu_start(self.device, self.cpu_index)

    def stop(self):
        """
        It stops code execution
        @exception OSError from C library errors
        """
        self.libwrnc.wrnc_cpu_stop(self.device, self.cpu_index)

    def is_running(self):
        """
        It checks if the CPU is running (or not)
        @exception OSError from C library errors
        """
        run = c_int(0)
        self.libwrnc.wrnc_cpu_is_running(self.device, self.cpu_index,
                                         byref(run))
        return True if run.value else False

    def load_application_file(self, file_path):
        """
        It loads a firmware from the given file
        @param[in] file_path path to the firmware file
        @exception OSError from C library errors
        """
        self.libwrnc.wrnc_cpu_load_application_file(self.device,
                                                    self.cpu_index,
                                                    file_path.encode())

    def dump_application_file(self, file_path):
        """
        It dumps the running firmware to the given file
        @param[in] file_path path to the firmware file
        @exception OSError from C library errors
        """
        self.libwrnc.wrnc_cpu_dump_application_file(self.device,
                                                    self.cpu_index,
                                                    file_path.encode())


class WrncHmq(Wrnc):
    """
    Python wrapper for HMQ management
    """
    FLAGS_DIR_OUT = 0x1
    FLAGS_DIR_IN = 0x0

    def __init__(self, lun=None, device_id=None,
                 hmq_index=0, flags=0):
        super(WrncHmq, self).__init__(lun, device_id)
        self.hmq_index = hmq_index
        self.flags = flags

        # Arguments
        self.libwrnc.wrnc_hmq_open.argtypes = [c_void_p, c_uint, c_ulong]
        self.libwrnc.wrnc_hmq_close.argtypes = [c_void_p]
        self.libwrnc.wrnc_hmq_receive.argtypes = [c_void_p]
        self.libwrnc.wrnc_hmq_send.argtypes = [c_void_p,
                                               POINTER(WrncMessageInternal)]
        self.libwrnc.wrnc_hmq_send_and_receive_sync\
            .argtypes = [c_void_p, c_uint, POINTER(WrncMessageInternal),
                         c_uint]
        self.libwrnc.wrnc_hmq_filter_add.argtypes = [c_void_p]
        self.libwrnc.wrnc_hmq_filter_clean.argtypes = [c_void_p]
        self.libwrnc.wrnc_message_pack.argtypes = [POINTER(WrncMessageInternal),
                                                   POINTER(WrncHeader),
                                                   c_void_p]
        self.libwrnc.wrnc_message_unpack.argtypes = [POINTER(WrncMessageInternal),
                                                    POINTER(WrncHeader),
                                                    c_void_p]
        # Return
        self.libwrnc.wrnc_hmq_open.restype = c_void_p
        self.libwrnc.wrnc_hmq_close.restype = None
        self.libwrnc.wrnc_hmq_receive.restype = POINTER(WrncMessageInternal)
        self.libwrnc.wrnc_hmq_send.restype = c_int
        self.libwrnc.wrnc_hmq_send_and_receive_sync.restype = c_int
        self.libwrnc.wrnc_hmq_filter_add.restype = c_int
        self.libwrnc.wrnc_hmq_filter_clean.restype = c_int
        # Error
        self.libwrnc.wrnc_hmq_open.errcheck = self.errcheck
        self.libwrnc.wrnc_hmq_close.errcheck = self.errcheck
        self.libwrnc.wrnc_hmq_receive.errcheck = self.errcheck
        self.libwrnc.wrnc_hmq_send.errcheck = self.errcheck
        self.libwrnc.wrnc_hmq_send_and_receive_sync.errcheck = self.errcheck
        self.libwrnc.wrnc_hmq_filter_add.errcheck = self.errcheck
        self.libwrnc.wrnc_hmq_filter_clean.errcheck = self.errcheck

        self.hmq = self.libwrnc.wrnc_hmq_open(self.device, self.hmq_index,
                                              flags)

    def __del__(self):
        self.libwrnc.wrnc_hmq_close(self.hmq)
        super(WrncHmq, self).__def__()

    def send_msg(self, header, payload, timeout=-1):
        """
        It sends an asynchronous message
        @param[in] header message header (len will be overwritte with the
                   computer payload length)
        @param[in] msg_list list of 32bit words to send
        @param[in] timeout time to wait before returning
        @exception OSError from C library errors
        """
        p = (c_uint32 * len(payload))(*payload)
        fmsg = WrncMessageInternal()
        header.len = len(payload)
        self.libwrnc.wrnc_message_pack(fmsg, header, pointer(p))
        self.libwrnc.wrnc_hmq_send(self.hmq, fmsg)

    def recv_msg(self, timeout=-1):
        """
        It receives an asynchronous message
        @param[in] timeout time to wait before returning
        @return the synchronous answer as list of 32bit words
        @exception OSError from C library errors
        """
        # TODO test me when you can
        set_errno(0)
        fmsg = self.libwrnc.wrnc_hmq_receive(self.hmq)

        if fmsg is None:
            raise OSError()

        h = WrncHeader()
        p2 = (c_uint32 * 128)()
        self.libwrnc.wrnc_message_unpack(fmsg, byref(h), byref(p2))
        return h, list(p)

    def sync_msg(self, header, payload, timeout=1000):
        """
        It sends a synchronous message
        @param[in] hmq_out index of the HMQ output slot
        @param[in] msg_list list of 32bit words to send
        @param[in] timeout time to wait before returning
        @return the synchronous answer as a set of header and payload
        @exception OSError from C library errors
        """

        p = (c_uint32 * len(payload))(*payload)
        fmsg = WrncMessageInternal()
        header.len = len(payload)
        self.libwrnc.wrnc_message_pack(fmsg, header, pointer(p))
        self.libwrnc.wrnc_hmq_send_and_receive_sync(self.hmq,
                                                    header.slot_io,
                                                    fmsg, timeout)
        h = WrncHeader()
        p2 = (c_uint32 * 128)()
        self.libwrnc.wrnc_message_unpack(fmsg, byref(h), byref(p2))
        return h, list(p2)


class WrncSmem(Wrnc):
    """
    Python wrapper for Shared Memory management
    """
    MOD_DIRECT = 0
    MOD_ADD = 1
    MOD_SUB = 2
    MOD_OR = 3
    MOD_CLR_AND = 4
    MOD_XOR = 5

    def __init__(self, lun=None, device_id=None):
        super(WrncSmem, self).__init__(lun, device_id)

        # Arguments - FIXME c_void_p should be point c_int
        self.libwrnc.wrnc_smem_read.argtypes = [c_void_p, c_uint,
                                                POINTER(c_int), c_size_t,
                                                c_uint]
        self.libwrnc.wrnc_smem_write.argtypes = [c_void_p, c_uint,
                                                 POINTER(c_int), c_size_t,
                                                 c_uint]
        # Return
        self.libwrnc.wrnc_smem_read.restype = c_int
        self.libwrnc.wrnc_smem_write.restype = c_int
        # Error
        self.libwrnc.wrnc_smem_read.errcheck = self.errcheck
        self.libwrnc.wrnc_smem_write.errcheck = self.errcheck

    def read(self, address, count):
        """
        It reads from the shared memory 'count' 32bit words starting
        from 'address'
        @param[in] address memory address where start writing
        @param[in] count number of 32bit words to read
        @return a list of integer
        @exception OSError from C library errors
        """
        dataArr = (c_int * count)()
        dataCas = cast(dataArr, POINTER(c_int))
        self.libwrnc.wrnc_smem_read(self.device, address, dataCas, count, 0)
        return list(dataArr)

    def write(self, address, values, modifer):
        """
        It writes 'values' to the shared memory starting from 'address'.
        @param[in] address memory address where start writing
        @param[in] values integer list to write
        @param[in] modifier write operation modifier. It changes the write
                   behaviour
        @exception OSError from C library errors
        """
        dataArr = (c_int * len(values))(*values)
        dataCas = cast(dataArr, POINTER(c_int))
        self.libwrnc.wrnc_smem_write(self.device, address, dataCas,
                                     len(values), modifer)

    def write_direct(self, address, values):
        self.write(address, values, self.MOD_DIRECT)

    def write_add(self, address, values):
        self.write(address, values, self.MOD_ADD)

    def write_sub(self, address, values):
        self.write(address, values, self.MOD_SUB)

    def write_or(self, address, values):
        self.write(address, values, self.MOD_OR)

    def write_clr_and(self, address, values):
        self.write(address, values, self.MOD_CLR_AND)

    def write_xor(self, address, values):
        self.write(address, values, self.MOD_XOR)


class WrncDebug(Wrnc):
    """
    Python wrapper for Debug Interface management
    """
    def __init__(self, lun=None, device_id=None,
                 cpu_index=0):
        super(WrncDebug, self).__init__(lun, device_id)
        self.cpu_index = cpu_index

        # Arguments - FIXME c_void_p should be point c_int
        self.libwrnc.wrnc_debug_open.argtypes = [c_void_p, c_uint]
        self.libwrnc.wrnc_debug_close.argtypes = [c_void_p]
        self.libwrnc.wrnc_debug_message_get.argtypes = [c_void_p,
                                                        c_char_p,
                                                        c_int]
        # Return
        self.libwrnc.wrnc_debug_open.restype = c_void_p
        self.libwrnc.wrnc_debug_close.restype = None
        self.libwrnc.wrnc_debug_message_get.restype = c_int
        # Error
        self.libwrnc.wrnc_debug_open.errcheck = self.errcheck
        self.libwrnc.wrnc_debug_message_get.errcheck = self.errcheck

        self.dbg = self.libwrnc.wrnc_debug_open(self.device, self.cpu_index)

    def __del__(self):
        super(WrncDEbug, self).__del__()
        self.libwrnc.wrnc_debug_close(self.dbg)

    def read(self):
        """
        It returns a string from the debug interface
        @return a String containing a debug message
        @exception OSError from C library errors
        """
        bsize = 1024
        buf = create_string_buffer(bsize)
        self.libwrnc.wrnc_debug_message_get(self.dbg, buf, bsize)
        return buf.value
