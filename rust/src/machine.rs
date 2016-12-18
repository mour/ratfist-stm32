
macro_rules! state_machine {
    (
        $mach_name:ident;
        $state_enum:ident;
        $event_enum:ident;
        [$($initial_state:tt)+];
        [$(
            [ $($state_name:tt)+ ] => [
                $( [ $($incoming_event:tt)+ ] [ $($new_state:tt)+ ] | $action:block ),+
            ]
        ),*]
    ) => (

        #[allow(dead_code)]
        #[derive(Debug)]
        pub struct $mach_name {
            current_state: $state_enum
        }

        impl $mach_name {
            pub fn new() -> $mach_name {
                $mach_name {
                    current_state: $initial_state
                }
            }

            pub fn process_event(&mut self, event: $event_enum) {
                match Some(&self.current_state) {
                    $(Some(&$state_name) => {
                        match Some(&event) {
                            $(Some(&$($incoming_event)+) => { {$action}; self.current_state = $new_state }),+
                            _ => {}
                        }
                    }),*
                    _ => {}
                }
            }
        }
    )

}
