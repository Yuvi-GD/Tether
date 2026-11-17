package com.tether.sandbox;

import android.app.NativeActivity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;

public class SandboxActivity extends NativeActivity {
    private static final int PICK_DOCUMENT = 1;
    private native void onDocumentPicked(String path);

    public void pickDocument() {
        runOnUiThread(() -> {
            try {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("*/*");
                startActivityForResult(intent, PICK_DOCUMENT);
            } catch (RuntimeException error) {
                onDocumentPicked(null);
            }
        });
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != PICK_DOCUMENT) return;
        Uri uri = data != null ? data.getData() : null;
        if (resultCode != RESULT_OK || uri == null) {
            onDocumentPicked(null);
            return;
        }
        File file = new File(getCacheDir(), "selected-document");
        try (InputStream input = getContentResolver().openInputStream(uri);
             FileOutputStream output = new FileOutputStream(file)) {
            if (input == null) throw new IOException("Cannot open document");
            byte[] buffer = new byte[8192];
            int count;
            while ((count = input.read(buffer)) != -1) output.write(buffer, 0, count);
            onDocumentPicked(file.getAbsolutePath());
        } catch (IOException | SecurityException error) {
            file.delete();
            onDocumentPicked(null);
        }
    }
}
