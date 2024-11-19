// ioshelper.mm
#import <CoreBluetooth/CoreBluetooth.h>

@interface BluetoothPermissionHelper : NSObject<CBCentralManagerDelegate>
@property (nonatomic, strong) CBCentralManager *centralManager;
@end

@implementation BluetoothPermissionHelper
- (id)init {
    if (self = [super init]) {
        self.centralManager = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
    }
    return self;
}
- (void)centralManagerDidUpdateState:(CBCentralManager *)central {
    if (central.state == CBManagerStatePoweredOn) {
        [self.centralManager scanForPeripheralsWithServices:nil options:nil];
    }
}
@end

extern "C" void requestIOSBluetoothPermissions() {
    static BluetoothPermissionHelper *helper = [[BluetoothPermissionHelper alloc] init];
}
