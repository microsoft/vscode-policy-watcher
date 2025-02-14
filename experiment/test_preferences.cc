#include <CoreServices/CoreServices.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MANAGED_PREFS_DIR "/Library/Managed Preferences/"

// Store the filename we're watching
typedef struct {
    const char* target_file;
} WatchContext;

void fsevents_callback(ConstFSEventStreamRef streamRef,
                       void *clientCallBackInfo,
                       size_t numEvents,
                       void *eventPaths,
                       const FSEventStreamEventFlags eventFlags[],
                       const FSEventStreamEventId eventIds[])
{
    char **paths = (char **)eventPaths;
    WatchContext *context = (WatchContext *)clientCallBackInfo;
    
    for (size_t i = 0; i < numEvents; i++)
    {
        if (strstr(paths[i], context->target_file) != NULL) {
            printf("🔔 VS Code (OSS) managed preferences changed: %s\n", paths[i]);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <plist_filename>\n", argv[0]);
        printf("Example: %s com.visualstudio.code.oss.plist\n", argv[0]);
        return 1;
    }

    WatchContext context = { .target_file = argv[1] };
    printf("🔍 Monitoring for changes to: %s\n", argv[1]);

    CFStringRef path = CFSTR(MANAGED_PREFS_DIR);
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&path, 1, NULL);

    FSEventStreamContext stream_context = {0, &context, NULL, NULL, NULL};
    FSEventStreamRef stream = FSEventStreamCreate(NULL,
                                                  &fsevents_callback,
                                                  &stream_context,
                                                  pathsToWatch,
                                                  kFSEventStreamEventIdSinceNow,
                                                  1.0,
                                                  kFSEventStreamCreateFlagFileEvents);

    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    FSEventStreamSetDispatchQueue(stream, queue);
    FSEventStreamStart(stream);

    CFRunLoopRun(); // Keep running

    CFRelease(path);
    CFRelease(pathsToWatch);
    return 0;
}