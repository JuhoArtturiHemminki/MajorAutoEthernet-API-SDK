import ctypes
import numpy as np
import os

class MAEDevice:
    def __init__(self, lib_name="major_auto_ethernet"):
        dir_path = os.path.dirname(os.path.realpath(__file__))
        if os.name == 'nt':
            lib_path = os.path.join(dir_path, f"../build/{lib_name}.dll")
        else:
            lib_path = os.path.join(dir_path, f"../build/{lib_name}.so")
            
        if not os.path.exists(lib_path):
            if os.name == 'nt':
                lib_path = os.path.join(dir_path, f"{lib_name}.dll")
            else:
                lib_path = os.path.join(dir_path, f"{lib_name}.so")
                
        if not os.path.exists(lib_path):
            raise FileNotFoundError(f"MAE Binary missing: {lib_path}")
            
        self.lib = ctypes.CDLL(lib_path)
        
        self.lib.MAE_CreateContext.restype = ctypes.c_void_p
        self.lib.MAE_Encode.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint64), ctypes.c_uint64]
        self.lib.MAE_Decode.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_uint64), ctypes.POINTER(ctypes.c_uint8), ctypes.c_uint64, ctypes.c_uint64]
        self.lib.MAE_DestroyContext.argtypes = [ctypes.c_void_p]
        
        self.ctx = self.lib.MAE_CreateContext()

    def pipeline_ingress(self, bit_array: np.ndarray):
        length = len(bit_array)
        c_input = (ctypes.c_uint8 * length)(*bit_array)
        c_output = (ctypes.c_uint64 * length)()
        
        self.lib.MAE_Encode(self.ctx, c_input, c_output, length)
        
        ctx_struct_ptr = ctypes.cast(self.ctx, ctypes.POINTER(ctypes.c_uint64))
        structural_key = ctx_struct_ptr[0]
        
        return np.array(c_output, dtype=np.uint64), structural_key

    def pipeline_egress(self, wave_stream: np.ndarray, bit_length: int, structural_key: int):
        c_stream = (ctypes.c_uint64 * bit_length)(*wave_stream)
        c_out_bits = (ctypes.c_uint8 * bit_length)()
        
        self.lib.MAE_Decode(self.ctx, c_stream, c_out_bits, bit_length, structural_key)
        return np.array(c_out_bits, dtype=np.uint8)

    def __del__(self):
        if hasattr(self, 'ctx') and self.ctx:
            self.lib.MAE_DestroyContext(self.ctx)

if __name__ == "__main__":
    ingress_payload = np.array([0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1], dtype=np.uint8)
    
    try:
        engine = MAEDevice()
        waveform, key = engine.pipeline_ingress(ingress_payload)
        
        independent_decoder = MAEDevice()
        egress_payload = independent_decoder.pipeline_egress(waveform, len(ingress_payload), key)
        
        is_perfect_match = np.array_equal(ingress_payload, egress_payload)
        print(f"MATCH:{is_perfect_match}")
        
    except Exception as error_log:
        pass
