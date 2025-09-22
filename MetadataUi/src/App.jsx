import React, { useState, useEffect } from 'react';
import './App.css'

function App() {
    const [ids, setPageContent] = useState(null)
    const [loading, setLoading] = useState(true);

    useEffect(() => {
	fetch("http://127.0.0.1:8080/metadata")
	    .then(response => {
		if (!response.ok) {
		    throw new Error('Error: ${response.status}');
		}
		return response.text();
	    })
	    .then(pageData => {
		setPageContent(pageData);
		setLoading(false);
	    });		  
    }, []);

    if (loading) {
	return <div>Loading...</div>
    }
    
  return (
    <>
	<div dangerouslySetInnerHTML={{ __html: ids }}>	    
	</div>
    </>
  )
}

export default App
