package io.flutter.plugin.common;

import java.nio.ByteBuffer;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

/**
 * A {@link MethodCodec} using UTF-8 encoded JSON method calls and result envelopes.
 * Values supported as methods arguments and result payloads are those supported by
 * {@link JSONMessageCodec}.
 */
public final class JSONMethodCodec implements MethodCodec {
    public static final JSONMethodCodec INSTANCE = new JSONMethodCodec();

    private JSONMethodCodec() {
    }

    @Override
    public ByteBuffer encodeMethodCall(MethodCall methodCall) {
       try {
          final JSONObject map = new JSONObject();
          map.put("method", methodCall.method);
          map.put("args", JSONObject.wrap(methodCall.arguments));
          return JSONMessageCodec.INSTANCE.encodeMessage(map);
       } catch (JSONException e) {
          throw new IllegalArgumentException("Invalid JSON", e);
       }
    }

    @Override
    public MethodCall decodeMethodCall(ByteBuffer message) {
        try {
            final Object json = JSONMessageCodec.INSTANCE.decodeMessage(message);
            if (json instanceof JSONObject) {
                final JSONObject map = (JSONObject) json;
                final Object method = map.get("method");
                final Object arguments = map.get("args");
                if (method instanceof String) {
                    return new MethodCall((String) method, arguments);
                }
            }
            throw new IllegalArgumentException("Invalid method call: " + json);
        } catch (JSONException e) {
            throw new IllegalArgumentException("Invalid JSON", e);
        }
    }

    @Override
    public ByteBuffer encodeSuccessEnvelope(Object result) {
        return JSONMessageCodec.INSTANCE
            .encodeMessage(new JSONArray().put(JSONObject.wrap(result)));
    }

    @Override
    public ByteBuffer encodeErrorEnvelope(String errorCode, String errorMessage,
        Object errorDetails) {
        return JSONMessageCodec.INSTANCE.encodeMessage(new JSONArray()
            .put(errorCode)
            .put(errorMessage)
            .put(JSONObject.wrap(errorDetails)));
    }

    @Override
    public Object decodeEnvelope(ByteBuffer envelope) {
        try {
            final Object json = JSONMessageCodec.INSTANCE.decodeMessage(envelope);
            if (json instanceof JSONArray) {
                final JSONArray array = (JSONArray) json;
                if (array.length() == 1) {
                    return array.get(0);
                }
                if (array.length() == 3) {
                    final Object code = array.get(0);
                    final Object message = array.get(1);
                    final Object details = array.get(2);
                    if (code instanceof String && (message == null || message instanceof String)) {
                        throw new FlutterException((String) code, (String) message, details);
                    }
                }
            }
            throw new IllegalArgumentException("Invalid method call: " + json);
        } catch (JSONException e) {
            throw new IllegalArgumentException("Invalid JSON", e);
        }
    }
}
