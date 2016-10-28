#include <stdlib.h>
#include <errno.h>

#include <epicsTypes.h>

#include <regDev.h>

#include <iocsh.h>
#include <epicsExport.h>

#include "toscaElb.h"

#define TOSCA_EXTERN_DEBUG
#define TOSCA_DEBUG_NAME toscaElb
#include "toscaDebug.h"
epicsExportAddress(int, toscaElbDebug);

struct regDevice
{
    int dummy;
};

void toscaElbDevReport(regDevice *device, int level)
{
    printf("Tosca ELB ( local bus)\n");
}

int toscaElbDevRead(
    regDevice *device,
    size_t offset,
    unsigned int dlen,
    size_t nelem,
    void* pdata,
    int priority,
    regDevTransferComplete callback,
    const char* user)
{
    int i;
    
    if (dlen == 0) return 0; /* any way to check online status ? */
    if (dlen != 4)
    {
        error("%s %s: only 4 bytes supported", user, regDevName(device));
        return -1;
    }
    if (offset & 3)
    {
        error("%s %s: offset must be multiple of 4", user, regDevName(device));
        return -1;
    }
    for (i = 0; i < nelem; i++)
    {
        ((epicsUInt32*)pdata)[i] = toscaElbRead(offset+(i<<2));
    }
    return 0;
}

int toscaElbDevWrite(
    regDevice *device,
    size_t offset,
    unsigned int dlen,
    size_t nelem,
    void* pdata,
    void* pmask,
    int priority,
    regDevTransferComplete callback,
    const char* user)
{
    int i;
    
    if (dlen != 4)
    {
        error("%s %s: only 4 bytes supported", user, regDevName(device));
        return -1;
    }
    if (offset & 3)
    {
        error("%s %s: offset must be multiple of 4", user, regDevName(device));
        return -1;
    }
    for (i = 0; i < nelem; i++)
    {
        toscaElbWrite(offset+(i<<2), ((epicsUInt32*)pdata)[i]);
    }
    return 0;
}

struct regDevSupport toscaElbDevRegDev = {
    .report = toscaElbDevReport,
    .read = toscaElbDevRead,
    .write = toscaElbDevWrite,
};

int toscaElbDevConfigure(const char* name)
{
    regDevice *device = NULL;
    
    if (!name || !name[0])
    {
        printf("usage: toscaElbDevConfigure name\n");
        return -1;
    }
    device = malloc(sizeof(regDevice));
    if (!device)
    {
        perror("malloc regDevice failed");
        return -1;
    }
    errno = 0;
    if (regDevRegisterDevice(name, &toscaElbDevRegDev, device, 0x44) != SUCCESS)
    {
        if (errno) perror("regDevRegisterDevice failed");
        free(device);
        return -1;
    }
    if (regDevInstallWorkQueue(device, 100) != SUCCESS)
    {
        perror("regDevInstallWorkQueue() failed");
        return -1;
    }
    return 0;
}

static const iocshFuncDef toscaElbReadDef =
    { "toscaElbRead", 1, (const iocshArg *[]) {
    &(iocshArg) { "address", iocshArgInt },
}};

static void toscaElbReadFunc(const iocshArgBuf *args)
{
    int address = args[0].ival;
    int value;
    errno = 0;
    value = toscaElbRead(address);
    if (value == -1 && errno != 0)
        error("toscaElbRead %s: %m", toscaElbAddrToRegname(address));
    else
        printf("0x%08x\n", value);
}

static const iocshFuncDef toscaElbWriteDef =
    { "toscaElbWrite", 2, (const iocshArg *[]) {
    &(iocshArg) { "address", iocshArgInt },
    &(iocshArg) { "value", iocshArgInt },
}};

static void toscaElbWriteFunc(const iocshArgBuf *args)
{
    int address = args[0].ival;
    int value = args[1].ival;
    if (toscaElbWrite(address, value) == -1)
        error("toscaElbWrite %s: %m", toscaElbAddrToRegname(address));
}

static const iocshFuncDef toscaElbDevConfigureDef =
    { "toscaElbDevConfigure", 1, (const iocshArg *[]) {
    &(iocshArg) { "name", iocshArgString },
}};

static void toscaElbDevConfigureFunc(const iocshArgBuf *args)
{
    toscaElbDevConfigure(args[0].sval);
}

static void toscaElbRegistrar(void)
{
    iocshRegister(&toscaElbDevConfigureDef, toscaElbDevConfigureFunc);
    iocshRegister(&toscaElbReadDef, toscaElbReadFunc);
    iocshRegister(&toscaElbWriteDef, toscaElbWriteFunc);
}

epicsExportRegistrar(toscaElbRegistrar);