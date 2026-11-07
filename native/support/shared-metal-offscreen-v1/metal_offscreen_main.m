/* Owned synthetic Mac probe for the exact diagnostic shader. */
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <dispatch/dispatch.h>
#include <stdio.h>
#include <string.h>
#include "shell_policy.h"

int main(int argc, const char *argv[]) {
    @autoreleasepool {
        if (argc != 2) return 2;
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device) {
            puts("Metal device unavailable on this Mac runner");
            return 3;
        }
        NSError *error = nil;
        NSURL *libraryURL = [NSURL fileURLWithPath:[NSString stringWithUTF8String:argv[1]]];
        id<MTLLibrary> library = [device newLibraryWithURL:libraryURL error:&error];
        id<MTLFunction> vertex = [library newFunctionWithName:@"md_vertex"];
        id<MTLFunction> fragment = [library newFunctionWithName:@"md_fragment"];
        if (!vertex || !fragment) {
            NSLog(@"Shader entries unavailable: %@", error);
            return 4;
        }
        MTLRenderPipelineDescriptor *description = [[MTLRenderPipelineDescriptor alloc] init];
        description.vertexFunction = vertex;
        description.fragmentFunction = fragment;
        description.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        id<MTLRenderPipelineState> pipeline = [device newRenderPipelineStateWithDescriptor:description error:&error];
        MTLTextureDescriptor *surface = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:64 height:64 mipmapped:NO];
        surface.storageMode = MTLStorageModePrivate;
        surface.usage = MTLTextureUsageRenderTarget;
        id<MTLTexture> texture = [device newTextureWithDescriptor:surface];
        id<MTLBuffer> output = [device newBufferWithLength:64u*256u options:MTLResourceStorageModeShared];
        id<MTLCommandQueue> queue = [device newCommandQueue];
        id<MTLCommandBuffer> command = [queue commandBuffer];
        if (!pipeline || !texture || !output.contents || !command) {
            NSLog(@"Metal resource creation refused: %@", error);
            return 5;
        }
        memset(output.contents, 0xa5, output.length);
        MTLRenderPassDescriptor *pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = texture;
        pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore;
        pass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 1, 1);
        id<MTLRenderCommandEncoder> draw = [command renderCommandEncoderWithDescriptor:pass];
        if (!draw) return 6;
        [draw setRenderPipelineState:pipeline];
        [draw setCullMode:MTLCullModeNone];
        [draw drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
        [draw endEncoding];
        id<MTLBlitCommandEncoder> blit = [command blitCommandEncoder];
        if (!blit) return 7;
        [blit copyFromTexture:texture sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
            sourceSize:MTLSizeMake(64, 64, 1) toBuffer:output destinationOffset:0
            destinationBytesPerRow:256 destinationBytesPerImage:64u*256u];
        [blit endEncoding];
        dispatch_semaphore_t done = dispatch_semaphore_create(0);
        [command addCompletedHandler:^(id<MTLCommandBuffer> completed) {
            (void)completed;
            dispatch_semaphore_signal(done);
        }];
        [command commit];
        if (dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, 7*NSEC_PER_SEC))) {
            puts("Metal completion timed out");
            return 8;
        }
        if (command.status != MTLCommandBufferStatusCompleted || command.error) {
            NSLog(@"Metal completion failed: %@", command.error);
            return 9;
        }
        if (!md_pixels_match(output.contents, output.length)) {
            puts("Metal offscreen samples differ");
            return 10;
        }
        puts("PASS Mac Metal command completed and both offscreen pixels matched");
        return 0;
    }
}
