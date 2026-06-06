#include <stdint.h>
#include <stdlib.h>

#define SHIFT 16
#define SCALE (1ULL << SHIFT)
#define MASK_64 ((1ULL << 60) - 1)

typedef struct {
    uint64_t reg_rx;
    uint64_t reg_rwa;
    uint64_t reg_rwb;
    uint64_t eta;
    uint64_t c_coeff;
} MAEContext;

#ifdef _WIN32
__declspec(dllexport) MAEContext* MAE_CreateContext() {
#else
MAEContext* MAE_CreateContext() {
#endif
    MAEContext* ctx = (MAEContext*)malloc(sizeof(MAEContext));
    if (!ctx) return NULL;
    ctx->reg_rx = (uint64_t)(0.1 * SCALE);
    ctx->reg_rwa = (uint64_t)(0.6 * SCALE);
    ctx->reg_rwb = (uint64_t)(0.4 * SCALE);
    ctx->eta = (uint64_t)(0.005 * SCALE);
    ctx->c_coeff = (uint64_t)(0.5 * SCALE);
    return ctx;
}

#ifdef _WIN32
__declspec(dllexport) void MAE_Encode(MAEContext* ctx, const uint8_t* input_bits, uint64_t* output_stream, uint64_t length) {
#else
void MAE_Encode(MAEContext* ctx, const uint8_t* input_bits, uint64_t* output_stream, uint64_t length) {
#endif
    for (uint64_t i = 0; i < length; i++) {
        uint64_t bit = input_bits[i];
        uint64_t x_w_b = (ctx->reg_rx * ctx->reg_rwb) >> SHIFT;
        uint64_t mod = (ctx->c_coeff * (((x_w_b * x_w_b) >> SHIFT) * x_w_b) >> SHIFT) >> SHIFT;
        
        uint64_t new_rx = ((ctx->reg_rx * ctx->reg_rwa) >> SHIFT) + mod + (bit << SHIFT);
        new_rx &= MASK_64;
        
        ctx->reg_rwa = (ctx->reg_rwa + ((ctx->eta * ((new_rx * ctx->reg_rx) >> SHIFT)) >> SHIFT)) & MASK_64;
        ctx->reg_rwb = (ctx->reg_rwb - ((ctx->eta * ((new_rx * mod) >> SHIFT)) >> SHIFT)) & MASK_64;
        
        output_stream[i] = ctx->reg_rx;
        ctx->reg_rx = new_rx;
    }
}

#ifdef _WIN32
__declspec(dllexport) void MAE_Decode(MAEContext* ctx, const uint64_t* data_stream, uint8_t* output_bits, uint64_t length, uint64_t structural_key) {
#else
void MAE_Decode(MAEContext* ctx, const uint64_t* data_stream, uint8_t* output_bits, uint64_t length, uint64_t structural_key) {
#endif
    uint64_t next_rx = structural_key;
    
    for (int64_t i = (int64_t)length - 1; i >= 0; i--) {
        uint64_t target_x = data_stream[i];
        
        uint64_t x_w_b = (target_x * ctx->reg_rwb) >> SHIFT;
        uint64_t mod = (ctx->c_coeff * (((x_w_b * x_w_b) >> SHIFT) * x_w_b) >> SHIFT) >> SHIFT;
        
        uint64_t pred_rx_0 = (((target_x * ctx->reg_rwa) >> SHIFT) + mod + (0 << SHIFT)) & MASK_64;
        uint64_t pred_rx_1 = (((target_x * ctx->reg_rwa) >> SHIFT) + mod + (1 << SHIFT)) & MASK_64;
        
        int64_t err_0 = (int64_t)pred_rx_0 - (int64_t)next_rx;
        int64_t err_1 = (int64_t)pred_rx_1 - (int64_t)next_rx;
        if (err_0 < 0) err_0 = -err_0;
        if (err_1 < 0) err_1 = -err_1;
        
        if (err_0 < err_1) {
            output_bits[i] = 0;
        } else {
            output_bits[i] = 1;
        }
        next_rx = target_x;
    }
}

#ifdef _WIN32
__declspec(dllexport) void MAE_DestroyContext(MAEContext* ctx) {
#else
void MAE_DestroyContext(MAEContext* ctx) {
#endif
    if (ctx) free(ctx);
}
