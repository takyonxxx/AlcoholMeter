package org.tbiliyor.alcoholmeter;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import androidx.core.app.ActivityCompat;
import android.app.Activity;
import org.qtproject.qt.android.bindings.QtActivity;
import android.util.Log;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.content.Intent;

public class MainActivity extends QtActivity {
    private static final int PERMISSION_REQUEST_CODE = 1;
    private static final int REQUEST_ENABLE_BT = 2;
    private static Activity qtActivity = null;
    private static final String TAG = "AlcoholMeter";
    private BluetoothAdapter bluetoothAdapter;

    @Override
    public void onCreate(android.os.Bundle savedInstanceState) {
        Log.d(TAG, "MainActivity onCreate called");
        super.onCreate(savedInstanceState);
        MainActivity.qtActivity = this;

        // Initialize Bluetooth
        BluetoothManager bluetoothManager = getSystemService(BluetoothManager.class);
        bluetoothAdapter = bluetoothManager.getAdapter();

        // Enable Bluetooth if needed
        if (!bluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        }

        checkAndRequestPermissions();
    }

    private void checkAndRequestPermissions() {
        String[] permissions;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            permissions = new String[] {
                Manifest.permission.BLUETOOTH_SCAN,
                Manifest.permission.BLUETOOTH_CONNECT,
                Manifest.permission.ACCESS_FINE_LOCATION,
                Manifest.permission.ACCESS_COARSE_LOCATION
            };
        } else {
            permissions = new String[] {
                Manifest.permission.BLUETOOTH,
                Manifest.permission.BLUETOOTH_ADMIN,
                Manifest.permission.ACCESS_FINE_LOCATION,
                Manifest.permission.ACCESS_COARSE_LOCATION
            };
        }

        boolean shouldRequest = false;
        for (String permission : permissions) {
            int result = checkSelfPermission(permission);
            Log.d(TAG, "Permission status for " + permission + ": " + (result == PackageManager.PERMISSION_GRANTED));
            if (result != PackageManager.PERMISSION_GRANTED) {
                shouldRequest = true;
            }
        }

        if (shouldRequest) {
            Log.d(TAG, "Requesting permissions in onCreate");
            requestPermissions(permissions, PERMISSION_REQUEST_CODE);
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_ENABLE_BT) {
            if (resultCode == RESULT_OK) {
                Log.d(TAG, "Bluetooth enabled successfully");
            } else {
                Log.e(TAG, "Bluetooth enable request denied");
            }
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        Log.d(TAG, "onRequestPermissionsResult: " + requestCode);

        if (requestCode == PERMISSION_REQUEST_CODE) {
            boolean allGranted = true;
            for (int i = 0; i < permissions.length; i++) {
                boolean granted = grantResults[i] == PackageManager.PERMISSION_GRANTED;
                Log.d(TAG, "Permission " + permissions[i] + ": " + (granted ? "GRANTED" : "DENIED"));
                if (!granted) allGranted = false;
            }

            if (allGranted) {
                Log.d(TAG, "All permissions granted, restarting Bluetooth operations");
                // Trigger any necessary Bluetooth operations here
            }
        }
    }

    public static void requestPermission(String permission) {
        Log.d(TAG, "Static requestPermission called with: " + permission);
        if (qtActivity == null) {
            try {
                Class<?> qtNativeClass = Class.forName("org.qtproject.qt.android.QtNative");
                java.lang.reflect.Method getActivityMethod = qtNativeClass.getMethod("activity");
                Activity activity = (Activity) getActivityMethod.invoke(null);
                if (activity != null) {
                    qtActivity = activity;
                    Log.d(TAG, "Activity retrieved from QtNative");
                }
            } catch (Exception e) {
                Log.e(TAG, "Failed to get activity: " + e.getMessage());
                return;
            }
        }

        if (qtActivity != null) {
            ((MainActivity)qtActivity).checkAndRequestPermissions();
        }
    }
}
