#pragma once

void renderer_setup(int w, int h);

void renderer_submit_frame(void *data1, void *data2);

void renderer_draw();

void renderer_cleanup();