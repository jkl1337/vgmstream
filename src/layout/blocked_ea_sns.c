#include "layout.h"
#include "../coding/coding.h"
#include "../vgmstream.h"

/* EA SNS/SPS blocks */
void block_update_ea_sns(off_t block_offset, VGMSTREAM * vgmstream) {
    STREAMFILE* streamFile = vgmstream->ch[0].streamfile;
    uint32_t block_size, block_samples;
    size_t file_size = get_streamfile_size(streamFile);
    off_t channel_start;
    size_t channel_interleave;
    int i;

    /* always BE */
    block_size    = read_32bitBE(block_offset + 0x00,streamFile);
    block_samples = read_32bitBE(block_offset + 0x04,streamFile);

    /* EOF */
    if (block_size == 0 || block_offset >= file_size) {
        vgmstream->current_block_offset = file_size;
        vgmstream->next_block_offset = file_size + 0x04;
        vgmstream->current_block_samples = vgmstream->num_samples;
        return;
    }

    /* At 0x00(1): block flag
     * - in SNS: 0x00=normal block, 0x80=last block (not mandatory)
     * - in SPS: 0x48=header, 0x44=normal block, 0x45=last block (empty) */
    block_size &= 0x00FFFFFF;

    switch(vgmstream->coding_type) {
        case coding_NGC_DSP:
            /* 0x04: unknown (0x00/02), 0x08: some size?, 0x34: null? */
            channel_start      = read_32bitBE(block_offset+0x08+0x00,streamFile);
            channel_interleave = read_32bitBE(block_offset+0x08+0x0c,streamFile);
            /* guessed as all known EA DSP only have one block with subheader (maybe changes coefs every block?) */
            if (channel_start >= 0x40) {
                dsp_read_coefs_be(vgmstream,streamFile, block_offset+0x08+0x10,0x28);
                dsp_read_hist_be (vgmstream,streamFile, block_offset+0x08+0x30,0x28);//todo guessed and doesn't fix clicks in full loops
            }
            break;

        default:
            channel_start = 0x00;
            channel_interleave = 0x00;
            break;
    }

    for (i = 0; i < vgmstream->channels; i++) {
        vgmstream->ch[i].offset = block_offset + 0x08 + channel_start + i*channel_interleave;

        /* also fix first offset (for EALayer3) */
        if (block_offset == vgmstream->ch[i].channel_start_offset) {
            vgmstream->ch[i].channel_start_offset = vgmstream->ch[i].offset;
        }
    }

    vgmstream->current_block_offset = block_offset;
    vgmstream->next_block_offset = block_offset + block_size;
    vgmstream->current_block_samples = block_samples;
}
