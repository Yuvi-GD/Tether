package com.tether.sandbox;

import android.app.NativeActivity;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.util.Locale;

public class SandboxActivity extends NativeActivity {
    private static final int PICK_DOCUMENT = 1;
    private static final long MAX_DOCUMENT_BYTES = 2L * 1024L * 1024L;
    private volatile String activeFilterExt = "";
    private native void onDocumentPicked(String path);

    public void pickDocument(String filterExt) {
        activeFilterExt = filterExt != null ? filterExt : "";
        runOnUiThread(() -> {
            try {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("*/*");
                startActivityForResult(intent, PICK_DOCUMENT);
            } catch (Exception error) {
                onDocumentPicked(null);
            }
        });
    }

    private boolean matchesFilter(String displayName) {
        String filter = activeFilterExt;
        if (filter == null || filter.trim().isEmpty()) return true;
        if (displayName == null) return false;
        String lower = displayName.toLowerCase(Locale.US);
        String[] exts = filter.split(",");
        for (String raw : exts) {
            String ext = raw.trim().toLowerCase(Locale.US);
            if (ext.isEmpty()) continue;
            if (!ext.startsWith(".")) ext = "." + ext;
            if (lower.endsWith(ext)) return true;
        }
        return false;
    }

    private boolean isValidDocument(Uri uri) {
        String displayName = null;
        long size = -1L;
        try (Cursor cursor = getContentResolver().query(
                uri,
                new String[] { OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE },
                null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int nameIdx = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (nameIdx >= 0 && !cursor.isNull(nameIdx)) {
                    displayName = cursor.getString(nameIdx);
                }
                int sizeIdx = cursor.getColumnIndex(OpenableColumns.SIZE);
                if (sizeIdx >= 0 && !cursor.isNull(sizeIdx)) {
                    size = cursor.getLong(sizeIdx);
                }
            }
        } catch (Exception ignored) {
            return false;
        }
        if (size > MAX_DOCUMENT_BYTES) return false;
        return matchesFilter(displayName);
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
        if (!isValidDocument(uri)) {
            onDocumentPicked(null);
            return;
        }
        File file = new File(getCacheDir(), "selected-document");
        try (InputStream input = getContentResolver().openInputStream(uri);
             FileOutputStream output = new FileOutputStream(file)) {
            if (input == null) throw new IOException("Cannot open document");
            byte[] buffer = new byte[8192];
            long total = 0L;
            int count;
            while ((count = input.read(buffer)) != -1) {
                total += count;
                if (total > MAX_DOCUMENT_BYTES) {
                    throw new IOException("Document exceeds size limit");
                }
                output.write(buffer, 0, count);
            }
            onDocumentPicked(file.getAbsolutePath());
        } catch (Exception error) {
            file.delete();
            onDocumentPicked(null);
        }
    }
}
