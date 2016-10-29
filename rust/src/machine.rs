
macro_rules! state_machine {
    (
        $mach_name:ident;
        $initial_state:ident;
        [$($states:ident),+];
        [$($events:tt)+];
        [$( $state_name:ident => [ $( [$( $event:tt )+] $new_state:ident | $action:block ),+ ] ),*]
    ) => (

        mod $mach_name {
            #[allow(dead_code)]
            #[derive(Debug)]
            pub enum States {
                $($states),+
            }

            #[allow(dead_code)]
            #[derive(Debug)]
            pub enum Events {
                $($events)*
            }

            #[allow(dead_code)]
            #[derive(Debug)]
            pub struct StateMachine {
                current_state: States
            }

            impl StateMachine {
                pub fn new() -> StateMachine {
                    StateMachine {
                        current_state: States::$initial_state
                    }
                }

                pub fn process_event(&mut self, event: Events) {
                    match Some(&self.current_state) {
                        $(Some(&States::$state_name) => {
                            match Some(&event) {
                                $(Some(&Events::$($event)+) => { {$action}; self.current_state = States::$new_state }),+
                                _ => {}
                            }
                        }),*
                        _ => {}
                    }
                }
            }
        }
    )
}
