//
//  DataCollectionViewController.h
//  ArcShirt
//
//  Created by john on 4/5/15.
//  Copyright (c) 2015 CS 145 Team. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreBluetooth/CoreBluetooth.h>

@interface DataCollectionViewController : UICollectionViewController <CBCentralManagerDelegate, CBPeripheralDelegate>

@end
