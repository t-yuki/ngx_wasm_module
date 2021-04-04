mod test_cases;

use crate::test_cases::*;
use chrono::{DateTime, Utc};
use http::StatusCode;
use log::*;
use proxy_wasm::traits::*;
use proxy_wasm::types::*;

#[no_mangle]
pub fn _start() {
    proxy_wasm::set_log_level(LogLevel::Trace);
    proxy_wasm::set_root_context(|_| -> Box<dyn RootContext> {
        Box::new(TestHttpHostcalls { context_id: 0 })
    });
}

struct TestHttpHostcalls {
    context_id: u32,
}

impl TestHttpHostcalls {
    fn send_plain_response(&mut self, status: StatusCode, body: Option<String>) {
        if let Some(b) = body {
            self.send_http_response(status.as_u16() as u32, vec![], Some(b.as_bytes()))
        } else {
            self.send_http_response(status.as_u16() as u32, vec![], None)
        }
    }

    fn test_echo(&mut self, path: &str) {
        match &path as &str {
            "headers" => {
                let headers = self
                    .get_http_request_headers()
                    .iter()
                    .map(|(name, value)| format!("{}: {}", name, value))
                    .collect::<Vec<String>>()
                    .join("\r\n");

                self.send_plain_response(StatusCode::OK, Some(headers));
            }
            "status" => {
                match StatusCode::from_bytes(path.as_bytes()).map_err(|_| StatusCode::BAD_REQUEST) {
                    Ok(status) => self.send_plain_response(status, None),
                    Err(status) => self.send_plain_response(status, None),
                }
            }
            _ => {}
        }
    }

    fn exec(&mut self, uri: String) -> Action {
        /*
         * GET /t/<endpoint>/<path...>
         */
        let (endpoint, path);
        {
            let p = uri.strip_prefix("/t").unwrap_or_else(|| uri.as_str());
            let mut segments: Vec<&str> = p.split("/").collect::<Vec<&str>>().split_off(1);
            endpoint = segments.remove(0);
            path = segments.join("/");
        }

        match endpoint {
            "log" => test_log(path.as_str(), self),
            "log_current_time" => {
                let now: DateTime<Utc> = self.get_current_time().into();
                info!("now: {}", now)
            }
            "log_http_request_headers" => test_log_http_request_headers(path.as_str(), self),
            "send_http_response" => test_send_http_response(path.as_str(), self),
            "get_http_request_header" => test_get_http_request_header(path.as_str(), self),
            "echo" => self.test_echo(path.as_str()),
            _ => self.send_http_response(404, vec![], None),
        }

        Action::Continue
    }
}

impl RootContext for TestHttpHostcalls {
    fn get_type(&self) -> Option<ContextType> {
        Some(ContextType::HttpContext)
    }

    fn create_http_context(&self, context_id: u32) -> Option<Box<dyn HttpContext>> {
        Some(Box::new(TestHttpHostcalls { context_id }))
    }
}

impl Context for TestHttpHostcalls {}

impl HttpContext for TestHttpHostcalls {
    fn on_http_request_headers(&mut self, _: usize) -> Action {
        match self.get_http_request_header(":path") {
            Some(path) => self.exec(path),
            _ => Action::Continue,
        }
    }
}
